/* tinyplay.c
**
** Copyright 2011, The Android Open Source Project
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
**     * Redistributions of source code must retain the above copyright
**       notice, this list of conditions and the following disclaimer.
**     * Redistributions in binary form must reproduce the above copyright
**       notice, this list of conditions and the following disclaimer in the
**       documentation and/or other materials provided with the distribution.
**     * Neither the name of The Android Open Source Project nor the names of
**       its contributors may be used to endorse or promote products derived
**       from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY The Android Open Source Project ``AS IS'' AND
** ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
** ARE DISCLAIMED. IN NO EVENT SHALL The Android Open Source Project BE LIABLE
** FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
** DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
** SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
** CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
** LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
** OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
** DAMAGE.
*/

#include <tinyalsa/asoundlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>

#define OPTPARSE_IMPLEMENTATION
#include "optparse.h"

struct cmd {
    const char *filename;
    const char *filetype;
    unsigned int card;
    unsigned int device;
    int flags;
    struct pcm_config config;
    unsigned int bits;
};

void cmd_init(struct cmd *cmd)
{
    cmd->filename = NULL;
    cmd->filetype = NULL;
    cmd->card = 0;
    cmd->device = 0;
    cmd->flags = PCM_OUT;
    cmd->config.period_size = 1024;
    cmd->config.period_count = 2;
    cmd->config.channels = 2;
    cmd->config.rate = 48000;
    cmd->config.format = PCM_FORMAT_S16_LE;
    cmd->config.silence_threshold = cmd->config.period_size * cmd->config.period_count;
    cmd->config.silence_size = 0;
    cmd->config.stop_threshold = cmd->config.period_size * cmd->config.period_count;
    cmd->config.start_threshold = cmd->config.period_size;
    cmd->bits = 16;
}

#define ID_RIFF 0x46464952
#define ID_WAVE 0x45564157
#define ID_FMT  0x20746d66
#define ID_DATA 0x61746164

struct riff_wave_header {
    uint32_t riff_id;
    uint32_t riff_sz;
    uint32_t wave_id;
};

struct chunk_header {
    uint32_t id;
    uint32_t sz;
};

struct chunk_fmt {
    uint16_t audio_format;
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
};

struct ctx {
    struct pcm *pcm;

    struct riff_wave_header wave_header;
    struct chunk_header chunk_header;
    struct chunk_fmt chunk_fmt;

    FILE *file;
};

int ctx_init(struct ctx* ctx, const struct cmd *cmd)
{
    unsigned int bits = cmd->bits;
    struct pcm_config config = cmd->config;

    if (cmd->filename == NULL) {
        fprintf(stderr, "filename not specified\n");
        return -1;
    }
    if (strcmp(cmd->filename, "-") == 0) {
        ctx->file = stdin;
    } else {
        ctx->file = fopen(cmd->filename, "rb");
    }

    if (ctx->file == NULL) {
        fprintf(stderr, "failed to open '%s'\n", cmd->filename);
        return -1;
    }

    if ((cmd->filetype != NULL) && (strcmp(cmd->filetype, "wav") == 0)) {
        if (fread(&ctx->wave_header, sizeof(ctx->wave_header), 1, ctx->file) != 1){
            fprintf(stderr, "error: '%s' does not contain a riff/wave header\n", cmd->filename);
            fclose(ctx->file);
            return -1;
        }
        if ((ctx->wave_header.riff_id != ID_RIFF) ||
            (ctx->wave_header.wave_id != ID_WAVE)) {
            fprintf(stderr, "error: '%s' is not a riff/wave file\n", cmd->filename);
            fclose(ctx->file);
            return -1;
        }
        unsigned int more_chunks = 1;
        do {
            if (fread(&ctx->chunk_header, sizeof(ctx->chunk_header), 1, ctx->file) != 1){
                fprintf(stderr, "error: '%s' does not contain a data chunk\n", cmd->filename);
                fclose(ctx->file);
                return -1;
            }
            switch (ctx->chunk_header.id) {
            case ID_FMT:
                if (fread(&ctx->chunk_fmt, sizeof(ctx->chunk_fmt), 1, ctx->file) != 1){
                    fprintf(stderr, "error: '%s' has incomplete format chunk\n", cmd->filename);
                    fclose(ctx->file);
                    return -1;
                }
                /* If the format header is larger, skip the rest */
                if (ctx->chunk_header.sz > sizeof(ctx->chunk_fmt))
                    fseek(ctx->file, ctx->chunk_header.sz - sizeof(ctx->chunk_fmt), SEEK_CUR);
                break;
            case ID_DATA:
                /* Stop looking for chunks */
                more_chunks = 0;
                break;
            default:
                /* Unknown chunk, skip bytes */
                fseek(ctx->file, ctx->chunk_header.sz, SEEK_CUR);
            }
        } while (more_chunks);
        config.channels = ctx->chunk_fmt.num_channels;
        config.rate = ctx->chunk_fmt.sample_rate;
        bits = ctx->chunk_fmt.bits_per_sample;
    }

    if (bits == 8) {
        config.format = PCM_FORMAT_S8;
    } else if (bits == 16) {
        config.format = PCM_FORMAT_S16_LE;
    } else if (bits == 24) {
        config.format = PCM_FORMAT_S24_3LE;
    } else if (bits == 32) {
        config.format = PCM_FORMAT_S32_LE;
    } else {
        fprintf(stderr, "bit count '%u' not supported\n", bits);
        fclose(ctx->file);
        return -1;
    }

    ctx->pcm = pcm_open(cmd->card,
                        cmd->device,
                        cmd->flags,
                        &config);
    if (ctx->pcm == NULL) {
        fprintf(stderr, "failed to allocate memory for pcm\n");
        fclose(ctx->file);
        return -1;
    } else if (!pcm_is_ready(ctx->pcm)) {
        fprintf(stderr, "failed to open for pcm %u,%u\n", cmd->card, cmd->device);
        fclose(ctx->file);
        pcm_close(ctx->pcm);
        return -1;
    }

    return 0;
}

void ctx_free(struct ctx *ctx)
{
    if (ctx == NULL) {
        return;
    }
    if (ctx->pcm != NULL) {
        pcm_close(ctx->pcm);
    }
    if (ctx->file != NULL) {
        fclose(ctx->file);
    }
}

static int close = 0;

int play_sample(struct ctx *ctx);

void stream_close(int sig)
{
    /* allow the stream to be closed gracefully */
    signal(sig, SIG_IGN);
    close = 1;
}

void print_usage(const char *argv0)
{
    fprintf(stderr, "usage: %s file.wav [options]\n", argv0);
    fprintf(stderr, "options:\n");
    fprintf(stderr, "-D | --card   <card number>    The card to receive the audio\n");
    fprintf(stderr, "-d | --device <device number>  The device to receive the audio\n");
    fprintf(stderr, "-p | --period-size <size>      The size of the PCM's period\n");
    fprintf(stderr, "-n | --period-count <count>    The number of PCM periods\n");
    fprintf(stderr, "-i | --file-type <file-type >  The type of file to read (raw or wav)\n");
    fprintf(stderr, "-c | --channels <count>        The amount of channels per frame\n");
    fprintf(stderr, "-r | --rate <rate>             The amount of frames per second\n");
    fprintf(stderr, "-b | --bits <bit-count>        The number of bits in one sample\n");
    fprintf(stderr, "-M | --mmap                    Use memory mapped IO to play audio\n");
}

int main(int argc, char **argv)
{
    int c;
    struct cmd cmd;
    struct ctx ctx;
    struct optparse opts;
    struct optparse_long long_options[] = {
        { "card",         'D', OPTPARSE_REQUIRED },
        { "device",       'd', OPTPARSE_REQUIRED },
        { "period-size",  'p', OPTPARSE_REQUIRED },
        { "period-count", 'n', OPTPARSE_REQUIRED },
        { "file-type",    'i', OPTPARSE_REQUIRED },
        { "channels",     'c', OPTPARSE_REQUIRED },
        { "rate",         'r', OPTPARSE_REQUIRED },
        { "bits",         'b', OPTPARSE_REQUIRED },
        { "mmap",         'M', OPTPARSE_NONE     },
        { "help",         'h', OPTPARSE_NONE     },
        { 0, 0, 0 }
    };

    if (argc < 2) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    cmd_init(&cmd);
    optparse_init(&opts, argv);
    while ((c = optparse_long(&opts, long_options, NULL)) != -1) {
        switch (c) {
        case 'D':
            if (sscanf(opts.optarg, "%u", &cmd.card) != 1) {
                fprintf(stderr, "failed parsing card number '%s'\n", argv[1]);
                return EXIT_FAILURE;
            }
            break;
        case 'd':
            if (sscanf(opts.optarg, "%u", &cmd.device) != 1) {
                fprintf(stderr, "failed parsing device number '%s'\n", argv[1]);
                return EXIT_FAILURE;
            }
            break;
        case 'p':
            if (sscanf(opts.optarg, "%u", &cmd.config.period_size) != 1) {
                fprintf(stderr, "failed parsing period size '%s'\n", argv[1]);
                return EXIT_FAILURE;
            }
            break;
        case 'n':
            if (sscanf(opts.optarg, "%u", &cmd.config.period_count) != 1) {
                fprintf(stderr, "failed parsing period count '%s'\n", argv[1]);
                return EXIT_FAILURE;
            }
            break;
        case 'c':
            if (sscanf(opts.optarg, "%u", &cmd.config.channels) != 1) {
                fprintf(stderr, "failed parsing channel count '%s'\n", argv[1]);
                return EXIT_FAILURE;
            }
            break;
        case 'r':
            if (sscanf(opts.optarg, "%u", &cmd.config.rate) != 1) {
                fprintf(stderr, "failed parsing rate '%s'\n", argv[1]);
                return EXIT_FAILURE;
            }
            break;
        case 'i':
            cmd.filetype = opts.optarg;
            break;
        case 'M':
            cmd.flags |= PCM_MMAP;
            break;
        case 'h':
            print_usage(argv[0]);
            return EXIT_SUCCESS;
        case '?':
            fprintf(stderr, "%s\n", opts.errmsg);
            return EXIT_FAILURE;
        }
    }
    cmd.filename = optparse_arg(&opts);

    if (cmd.filename != NULL && cmd.filetype == NULL &&
        (cmd.filetype = strrchr(cmd.filename, '.')) != NULL) {
        cmd.filetype++;
    }

    cmd.config.silence_threshold = cmd.config.period_size * cmd.config.period_count;
    cmd.config.stop_threshold = cmd.config.period_size * cmd.config.period_count;
    cmd.config.start_threshold = cmd.config.period_size;

    if (ctx_init(&ctx, &cmd) < 0) {
        return EXIT_FAILURE;
    }

    /* TODO get parameters from context */
    printf("playing '%s': %u ch, %u hz, %u bit\n",
           cmd.filename,
           cmd.config.channels,
           cmd.config.rate,
           cmd.bits);

    if (play_sample(&ctx) < 0) {
        ctx_free(&ctx);
        return EXIT_FAILURE;
    }

    ctx_free(&ctx);
    return EXIT_SUCCESS;
}

int check_param(struct pcm_params *params, unsigned int param, unsigned int value,
                 char *param_name, char *param_unit)
{
    unsigned int min;
    unsigned int max;
    int is_within_bounds = 1;

    min = pcm_params_get_min(params, param);
    if (value < min) {
        fprintf(stderr, "%s is %u%s, device only supports >= %u%s\n", param_name, value,
                param_unit, min, param_unit);
        is_within_bounds = 0;
    }

    max = pcm_params_get_max(params, param);
    if (value > max) {
        fprintf(stderr, "%s is %u%s, device only supports <= %u%s\n", param_name, value,
                param_unit, max, param_unit);
        is_within_bounds = 0;
    }

    return is_within_bounds;
}

int sample_is_playable(const struct cmd *cmd)
{
    struct pcm_params *params;
    int can_play;

    params = pcm_params_get(cmd->card, cmd->device, PCM_OUT);
    if (params == NULL) {
        fprintf(stderr, "unable to open PCM %u,%u\n", cmd->card, cmd->device);
        return 0;
    }

    can_play = check_param(params, PCM_PARAM_RATE, cmd->config.rate, "sample rate", "hz");
    can_play &= check_param(params, PCM_PARAM_CHANNELS, cmd->config.channels, "sample", " channels");
    can_play &= check_param(params, PCM_PARAM_SAMPLE_BITS, cmd->bits, "bits", " bits");
    can_play &= check_param(params, PCM_PARAM_PERIOD_SIZE, cmd->config.period_size, "period size",
                            " frames");
    can_play &= check_param(params, PCM_PARAM_PERIODS, cmd->config.period_count, "period count",
                            " frames");

    pcm_params_free(params);

    return can_play;
}

int play_sample(struct ctx *ctx)
{
    char *buffer;
    size_t buffer_size = 0;
    size_t num_read = 0;
    size_t remaining_data_size = ctx->chunk_header.sz;
    size_t read_size = 0;
    const struct pcm_config *config = pcm_get_config(ctx->pcm);

    if (config == NULL) {
        fprintf(stderr, "unable to get pcm config\n");
        return -1;
    }

    buffer_size = pcm_frames_to_bytes(ctx->pcm, config->period_size);
    buffer = malloc(buffer_size);
    if (!buffer) {
        fprintf(stderr, "unable to allocate %zu bytes\n", buffer_size);
        return -1;
    }

    /* catch ctrl-c to shutdown cleanly */
    signal(SIGINT, stream_close);

    do {
        read_size = remaining_data_size > buffer_size ? buffer_size : remaining_data_size;
        num_read = fread(buffer, 1, read_size, ctx->file);
        if (num_read > 0) {
            if (pcm_writei(ctx->pcm, buffer,
                pcm_bytes_to_frames(ctx->pcm, num_read)) < 0) {
                fprintf(stderr, "error playing sample\n");
                break;
            }
            remaining_data_size -= num_read;
        }
    } while (!close && num_read > 0 && remaining_data_size > 0);

    pcm_wait(ctx->pcm, -1);

    free(buffer);
    return 0;
}

