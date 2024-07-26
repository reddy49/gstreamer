/* soc.h - Vx Sound SoC layer Header File */

/*
 * The source code in this file refers to or is derived from GPL-licensed
 * source code. It is strictly proprietary and must NOT be distributed
 * externally under any circumstances.
 */

/*
DESCRIPTION
This file defines the Vx Sound SoC functions and structures for the PCM module
*/

#ifndef __INCvxSndSoch
#define __INCvxSndSoch

/* includes */

#include <vxWorks.h>
#include <vsbConfig.h>
#include <ioLib.h>
#include <errnoLib.h>
#include <semLib.h>
#include <stdio.h>
#include <sysLib.h>
#include <dllLib.h>

#ifdef _WRS_KERNEL
#include <hwif/vxBus.h>
#include <hwif/vxbus/vxbLib.h>
#include <hwif/buslib/vxbFdtLib.h>
#include <subsys/reg/vxbRegMap.h>
#endif
#include <msgQLib.h>
#include <taskLib.h>
#include <tickLib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <ffsLib.h>
#include <vxSoundCore.h>
#include <control.h>
#include <pcm.h>

#if __cplusplus
extern "C" {
#endif /* __cplusplus  */

#ifdef _WRS_KERNEL

/* defines */

#define CONTAINER_OF(ptr, type, member)                                 \
        ((type*)((char *)(ptr) - OFFSET(type,member)))

/* for dummy codec */

#define SND_SOC_DUMMY_CODEC_COMPT_NAME      "snd-soc-dummy"
#define SND_SOC_DUMMY_CODEC_DAI_DRV_NAME    "snd-soc-dummy-dai"

/*
 * Component probe and remove ordering levels for components with runtime
 * dependencies.
 */
#define SND_SOC_COMP_ORDER_FIRST    -2
#define SND_SOC_COMP_ORDER_EARLY    -1
#define SND_SOC_COMP_ORDER_NORMAL    0
#define SND_SOC_COMP_ORDER_LATE      1
#define SND_SOC_COMP_ORDER_LAST      2

#define for_each_comp_order(order)          \
    for (order  = SND_SOC_COMP_ORDER_FIRST; \
         order <= SND_SOC_COMP_ORDER_LAST;  \
         order++)

/* control has no PM register bit */
#define SND_SOC_NOPM    -1

#define SNDRV_CTL_TLVT_CONTAINER        0  /* one level down - group of TLVs */
#define SNDRV_CTL_TLVT_DB_SCALE         1       /* dB scale */
#define SNDRV_CTL_TLVT_DB_LINEAR        2  /* linear volume */
#define SNDRV_CTL_TLVT_DB_RANGE         3   /* dB range container */
#define SNDRV_CTL_TLVT_DB_MINMAX        4  /* dB scale with min/max */
#define SNDRV_CTL_TLVT_DB_MINMAX_MUTE   5 /* dB scale with min/max with mute */

/*
 * DAI hardware audio formats.
 *
 * Describes the physical PCM data formating and clocking. Add new formats
 * to the end.
 */

#define VX_SND_DAI_FORMAT_I2S          1 /* I2S mode */
#define VX_SND_DAI_FORMAT_RIGHT_J      2 /* Right Justified mode */
#define VX_SND_DAI_FORMAT_LEFT_J       3 /* Left Justified mode */
#define VX_SND_DAI_FORMAT_DSP_A        4 /* L data MSB after FRM LRC */
#define VX_SND_DAI_FORMAT_DSP_B        5 /* L data MSB during FRM LRC */
#define VX_SND_DAI_FORMAT_AC97         6 /* AC97 */
#define VX_SND_DAI_FORMAT_PDM          7 /* Pulse density modulation */

#define SND_SOC_DAIFMT_I2S              VX_SND_DAI_FORMAT_I2S
#define SND_SOC_DAIFMT_RIGHT_J          VX_SND_DAI_FORMAT_RIGHT_J
#define SND_SOC_DAIFMT_LEFT_J           VX_SND_DAI_FORMAT_LEFT_J
#define SND_SOC_DAIFMT_DSP_A            VX_SND_DAI_FORMAT_DSP_A
#define SND_SOC_DAIFMT_DSP_B            VX_SND_DAI_FORMAT_DSP_B
#define SND_SOC_DAIFMT_AC97             VX_SND_DAI_FORMAT_AC97
#define SND_SOC_DAIFMT_PDM              VX_SND_DAI_FORMAT_PDM

#define VX_SND_SOC_POSSIBLE_DAIFMT_FORMAT_SHIFT 0
#define VX_SND_SOC_POSSIBLE_DAIFMT_FORMAT_MASK  (0xFFFF << VX_SND_SOC_POSSIBLE_DAIFMT_FORMAT_SHIFT)
#define VX_SND_SOC_POSSIBLE_DAIFMT_I2S      (1 << VX_SND_DAI_FORMAT_I2S)
#define VX_SND_SOC_POSSIBLE_DAIFMT_RIGHT_J      (1 << VX_SND_DAI_FORMAT_RIGHT_J)
#define VX_SND_SOC_POSSIBLE_DAIFMT_LEFT_J       (1 << VX_SND_DAI_FORMAT_LEFT_J)
#define VX_SND_SOC_POSSIBLE_DAIFMT_DSP_A        (1 << VX_SND_DAI_FORMAT_DSP_A)
#define VX_SND_SOC_POSSIBLE_DAIFMT_DSP_B        (1 << VX_SND_DAI_FORMAT_DSP_B)
#define VX_SND_SOC_POSSIBLE_DAIFMT_AC97     (1 << VX_SND_DAI_FORMAT_AC97)
#define VX_SND_SOC_POSSIBLE_DAIFMT_PDM      (1 << VX_SND_DAI_FORMAT_PDM)

#define VX_SND_SOC_POSSIBLE_DAIFMT_CLOCK_SHIFT  16
#define VX_SND_SOC_POSSIBLE_DAIFMT_CLOCK_MASK   (0xFFFF << VX_SND_SOC_POSSIBLE_DAIFMT_CLOCK_SHIFT)
#define VX_SND_SOC_POSSIBLE_DAIFMT_GATED        (0x1ULL << VX_SND_SOC_POSSIBLE_DAIFMT_CLOCK_SHIFT)
#define VX_SND_SOC_POSSIBLE_DAIFMT_CONT         (0x2ULL << VX_SND_SOC_POSSIBLE_DAIFMT_CLOCK_SHIFT)

#define VX_SND_SOC_POSSIBLE_DAIFMT_INV_SHIFT    32
#define VX_SND_SOC_POSSIBLE_DAIFMT_INV_MASK     (0xFFFFULL << VX_SND_SOC_POSSIBLE_DAIFMT_INV_SHIFT)
#define VX_SND_SOC_POSSIBLE_DAIFMT_NB_NF        (0x1ULL    << VX_SND_SOC_POSSIBLE_DAIFMT_INV_SHIFT)
#define VX_SND_SOC_POSSIBLE_DAIFMT_NB_IF        (0x2ULL    << VX_SND_SOC_POSSIBLE_DAIFMT_INV_SHIFT)
#define VX_SND_SOC_POSSIBLE_DAIFMT_IB_NF        (0x4ULL    << VX_SND_SOC_POSSIBLE_DAIFMT_INV_SHIFT)
#define VX_SND_SOC_POSSIBLE_DAIFMT_IB_IF        (0x8ULL    << VX_SND_SOC_POSSIBLE_DAIFMT_INV_SHIFT)

/*
 * DAI hardware clock providers/consumers
 *
 * This is wrt the codec, the inverse is true for the interface
 * i.e. if the codec is clk and FRM provider then the interface is
 * clk and frame consumer.
 */
#define SND_SOC_DAIFMT_CBP_CFP      (1 << 12) /* codec clk provider & frame provider */
#define SND_SOC_DAIFMT_CBC_CFP      (2 << 12) /* codec clk consumer & frame provider */
#define SND_SOC_DAIFMT_CBP_CFC      (3 << 12) /* codec clk provider & frame consumer */
#define SND_SOC_DAIFMT_CBC_CFC      (4 << 12) /* codec clk consumer & frame consumer */

/* previous definitions kept for backwards-compatibility, do not use in new contributions */
#define SND_SOC_DAIFMT_CBM_CFM      SND_SOC_DAIFMT_CBP_CFP
#define SND_SOC_DAIFMT_CBS_CFM      SND_SOC_DAIFMT_CBC_CFP
#define SND_SOC_DAIFMT_CBM_CFS      SND_SOC_DAIFMT_CBP_CFC
#define SND_SOC_DAIFMT_CBS_CFS      SND_SOC_DAIFMT_CBC_CFC

/*
 * DAI hardware signal polarity.
 *
 * Specifies whether the DAI can also support inverted clocks for the specified
 * format.
 *
 * BCLK:
 * - "normal" polarity means signal is available at rising edge of BCLK
 * - "inverted" polarity means signal is available at falling edge of BCLK
 *
 * FSYNC "normal" polarity depends on the frame format:
 * - I2S: frame consists of left then right channel data. Left channel starts
 *      with falling FSYNC edge, right channel starts with rising FSYNC edge.
 * - Left/Right Justified: frame consists of left then right channel data.
 *      Left channel starts with rising FSYNC edge, right channel starts with
 *      falling FSYNC edge.
 * - DSP A/B: Frame starts with rising FSYNC edge.
 * - AC97: Frame starts with rising FSYNC edge.
 *
 * "Negative" FSYNC polarity is the one opposite of "normal" polarity.
 */

#define SND_SOC_DAIFMT_NB_NF        (0 << 8) /* normal bit clock + frame */
#define SND_SOC_DAIFMT_NB_IF        (2 << 8) /* normal BCLK + inv FRM */
#define SND_SOC_DAIFMT_IB_NF        (3 << 8) /* invert BCLK + nor FRM */
#define SND_SOC_DAIFMT_IB_IF        (4 << 8) /* invert BCLK + FRM */

#define SND_SOC_DAIFMT_FORMAT_MASK          0x000f
#define SND_SOC_DAIFMT_CLOCK_MASK           0x00f0
#define SND_SOC_DAIFMT_INV_MASK             0x0f00
#define SND_SOC_DAIFMT_CLOCK_PROVIDER_MASK  0xf000

#define SND_SOC_DAIFMT_MASTER_MASK  SND_SOC_DAIFMT_CLOCK_PROVIDER_MASK

//#define snd_pcm_substream_chip(substream) ((substream)->private_data)
#define VX_SND_SUBSTREAM_CHIP(substream) ((substream)->privateData)
//#define snd_pcm_chip(pcm) ((pcm)->private_data)
#define VX_SND_PCM_CHIP(pcm) ((pcm)->privateData)

//#define asoc_rtd_to_cpu(rtd, n)   (rtd)->dais[n]
#define SOC_RTD_TO_CPU(runtime, n)   (runtime)->dais[n]
//#define asoc_rtd_to_codec(rtd, n) (rtd)->dais[n + (rtd)->num_cpus]
#define SOC_RTD_TO_CODEC(runtime, n) (runtime)->dais[n + (runtime)->cpuNum]
//#define asoc_substream_to_rtd(substream)
#define VX_SOC_SUBSTREAM_TO_RUNTIME(substream)                        \
        (struct vxSndSocPcmRuntime *) VX_SND_SUBSTREAM_CHIP(substream)

//for_each_rtd_components(rtd, i, component)
#define FOR_EACH_RUNTIME_COMPONENTS(runtime, i, component)            \
    for ((i) = 0, component = NULL;                                   \
         ((i) < runtime->componentNum) &&                             \
          ((component) = runtime->components[i]);                     \
         (i)++)

#define FOR_EACH_CARD_COMPONENTS(card, component)                       \
    for (component = CONTAINER_OF(DLL_FIRST(&card->component_dev_list), \
                                  struct vxSndSocComponent, cardNode);  \
         component != NULL;                                             \
         component = CONTAINER_OF(DLL_NEXT(component->cardNode),        \
                                  struct vxSndSocComponent, cardNode)

//#define for_each_rtd_cpu_dais(rtd, i, dai)
#define FOR_EACH_RUNTIME_CPU_DAIS(runtime, i, dai)                        \
    for ((i) = 0;                                                         \
         ((i) < runtime->cpuNum) && ((dai) = SOC_RTD_TO_CPU(runtime, i)); \
         (i)++)

//#define for_each_rtd_codec_dais(rtd, i, dai)
#define FOR_EACH_RUNTIME_CODEC_DAIS(runtime, i, dai)                      \
    for ((i) = 0;                                                         \
         ((i) < runtime->codecNum) && ((dai) = SOC_RTD_TO_CODEC(runtime, i)); \
         (i)++)

//#define for_each_rtd_dais(rtd, i, dai)
#define FOR_EACH_RUNTIME_DAIS(runtime, i, dai)              \
    for ((i) = 0;                                           \
         ((i) < (runtime)->cpuNum + (runtime)->codecNum) && \
             ((dai) = (runtime)->dais[i]);                  \
         (i)++)

//#define for_each_pcm_streams(stream)
#define FOR_EACH_PCM_STREAMS(direct)                        \
    for (direct  = SNDRV_PCM_STREAM_PLAYBACK;               \
         direct < SNDRV_PCM_STREAM_MAX; direct++)

#define FOR_EACH_SOC_CARD_RUNTIME(card, runtime)                            \
    for (runtime = (struct vxSndSocPcmRuntime *) DLL_FIRST (&card->rtd_list); \
         runtime != NULL;                                                    \
         runtime = (struct vxSndSocPcmRuntime *) DLL_NEXT ((DL_NODE *) runtime))

#define FOR_EACH_SOC_BE_RTD(fe, direct, be)     \
    for (be = fe->pBe[direct]; be != NULL; be = be->pBe[direct])

#define SOC_GET_SUBSTREAM_FROM_RTD(rtd, direct) \
    rtd->pcm->stream[direct].substream

//for_each_card_prelinks
#define FOR_EACH_CARD_PRELINKS(card, i, link)                           \
    for ((i) = 0;                                                       \
         ((i) < (card)->num_links) && ((link) = &(card)->dai_link[i]);  \
         (i)++)

struct vxSndSocComponent;
struct vxSndSocComponentDriver;
struct vxSndSocDai;
struct vxSndSocDaiDriver;
struct vxSndSocPcmStream;
struct vxSndSocDaiOps;
struct vxSndSocPcmRuntime;
struct snd_soc_dpcm_runtime;
struct vxSndSocDaiLink;
struct vxSndSocOps;
struct vxSndSocDaiLinkComponent;
struct vxSndSocCard;
struct snd_soc_codec_conf;
enum snd_soc_dpcm_state;

STATUS vxSndCtrlVolumeInfo (VX_SND_CONTROL * ctrl, VX_SND_CTRL_INFO * info);
STATUS vxSndCtrlVolumeGet (VX_SND_CONTROL * ctrl, VX_SND_CTRL_DATA_VAL * data);
STATUS vxSndCtrlVolumePut (VX_SND_CONTROL * ctrl, VX_SND_CTRL_DATA_VAL * data);
uint32_t vxSndCtrlEnumValToItem (VX_SND_ENUM * pEnum, uint32_t value);
uint32_t vxSndCtrlItemToEnumVal (VX_SND_ENUM * pEnum, uint32_t item);
STATUS snd_soc_info_enum_double (VX_SND_CONTROL * ctrl,
                                 VX_SND_CTRL_INFO * info);
STATUS snd_soc_get_enum_double (VX_SND_CONTROL * ctrl,
                                VX_SND_CTRL_DATA_VAL * data);
STATUS snd_soc_put_enum_double (VX_SND_CONTROL * ctrl,
                                VX_SND_CTRL_DATA_VAL * data);

// used as static DECLARE_TLV_DB_SCALE(dac_tlv, -10350, 50, 0);
#define DECLARE_TLV_DB_SCALE(name, min, step, mute) \
    uint32_t name[] = {SNDRV_CTL_TLV_DB_SCALE, \
                 sizeof(uint32_t) * 2, \
                 (uint32_t) (min & 0xffff),  \
                 (uint32_t) ((mute) != 0 ? 0x10000 : 0) | ((step) & 0xffff) }
//#define SOC_DOUBLE_VALUE(xreg, shift_left, shift_right, xmax, xinvert, xautodisable)
#define SOC_DOUBLE_VALUE(xreg, shift_left, shift_right, xmax, xinvert, xautodisable) \
            ((unsigned long)&(struct vxSndMixerControl) \
            {.reg = xreg, .rreg = xreg, .shift = shift_left, \
            .shiftRight = shift_right, .max = xmax, .platformMax = xmax, \
            .invert = xinvert, .autodisable = xautodisable})
//#define SOC_SINGLE_VALUE(xreg, xshift, xmax, xinvert, xautodisable)
#define SOC_SINGLE_VALUE(xreg, xshift, xmax, xinvert, xautodisable) \
        SOC_DOUBLE_VALUE(xreg, xshift, xshift, xmax, xinvert, xautodisable)

#define SOC_SINGLE(xname, reg, shift, max, invert) \
{   .id.iface = SNDRV_CTL_TYPE_MIXER, .id.name = xname, \
    .info = vxSndCtrlVolumeInfo, .get = vxSndCtrlVolumeGet,\
    .put = vxSndCtrlVolumePut, \
    .privateValue = SOC_SINGLE_VALUE(reg, shift, max, invert, 0) }

//#define SOC_SINGLE_TLV(xname, reg, shift, max, invert, tlv_array)
#define SOC_SINGLE_TLV(xname, reg, shift, max, invert, tlv_array) \
    {   \
    .id.iface = SNDRV_CTL_TYPE_MIXER, .id.name = xname, \
    .id.access = VX_SND_CTRL_ACCESS_TLV_READ | VX_SND_CTRL_ACCESS_READWRITE, \
    .tlv.p = (tlv_array), \
    .info = vxSndCtrlVolumeInfo, \
    .get = vxSndCtrlVolumeGet, \
    .put = vxSndCtrlVolumePut, \
    .privateValue = SOC_SINGLE_VALUE(reg, shift, max, invert, 0) }

//#define SOC_VALUE_ENUM_DOUBLE(xreg, xshift_l, xshift_r, xmask, xitems, xtexts, xvalues)
#define SOC_VALUE_ENUM_DOUBLE(xreg, xshift_l, xshift_r, xmask, xitems, xtexts, xvalues) \
    {   .reg = xreg, .shiftLeft = xshift_l, .shiftRight = xshift_r, \
        .mask = xmask, .itemNum = xitems, .textList = xtexts, .valueList = xvalues}

//#define SOC_VALUE_ENUM_SINGLE(xreg, xshift, xmask, xitems, xtexts, xvalues)
#define SOC_VALUE_ENUM_SINGLE(xreg, xshift, xmask, xitems, xtexts, xvalues) \
        PRI_VAL_VALUE_ENUM_DOUBLE(xreg, xshift, xshift, xmask, xitems, xtexts, xvalues)

//#define SOC_ENUM_DOUBLE(xreg, xshift_l, xshift_r, xitems, xtexts)
#define SOC_ENUM_DOUBLE(xreg, xshift_l, xshift_r, xitems, xtexts) \
    {   .reg = xreg, .shiftLeft = xshift_l, .shiftRight = xshift_r, \
        .itemNum = xitems, .textList = xtexts}
//#define SOC_ENUM_SINGLE(xreg, xshift, xitems, xtexts)
#define SOC_ENUM_SINGLE(xreg, xshift, xitems, xtexts) \
    SOC_ENUM_DOUBLE(xreg, xshift, xshift, xitems, xtexts)

#define SOC_ENUM_DOUBLE_DECL(name, xreg, xshift_l, xshift_r, xtexts) \
    VX_SND_ENUM name = SOC_ENUM_DOUBLE(xreg, xshift_l, xshift_r, \
                        NELEMENTS (xtexts), xtexts)
#define SOC_ENUM_SINGLE_DECL(name, xreg, xshift, xtexts) \
    SOC_ENUM_DOUBLE_DECL(name, xreg, xshift, xshift, xtexts)

#define SOC_ENUM_SINGLE_EXT(xitems, xtexts) \
    {   .itemNum = xitems, .textList = xtexts }

#define SOC_ENUM_SINGLE_EXT_DECL(name, xtexts) \
    const VX_SND_ENUM name = SOC_ENUM_SINGLE_EXT(NELEMENTS(xtexts), xtexts)

//#define SOC_ENUM_DOUBLE(xreg, xshift_l, xshift_r, xitems, xtexts)
#define SOC_ENUM(xname, xenum) \
{   .id.iface = SNDRV_CTL_TYPE_MIXER, .id.name = xname,\
    .info = snd_soc_info_enum_double, \
    .get = snd_soc_get_enum_double, .put = snd_soc_put_enum_double, \
    .privateValue = (unsigned long)&xenum }

#define SOC_ENUM_EXT(xname, xenum, xhandler_get, xhandler_put) \
{   .id.iface = SNDRV_CTL_TYPE_MIXER, .id.name = xname, \
    .info = snd_soc_info_enum_double, \
    .get = xhandler_get, .put = xhandler_put, \
    .privateValue = (unsigned long)&xenum }
#define SOC_VALUE_ENUM_EXT(xname, xenum, xhandler_get, xhandler_put) \
    SOC_ENUM_EXT(xname, xenum, xhandler_get, xhandler_put)

//static inline bool snd_soc_volsw_is_stereo(struct soc_mixer_control *mc)
#define MIXER_CTRL_IS_STEREO(mixer) \
    ((mixer)->reg != (mixer)->rreg || (mixer)->shift != (mixer)->shiftRight)

typedef enum snd_soc_dpcm_state
    {
    SND_SOC_DPCM_STATE_NEW = 0,
    SND_SOC_DPCM_STATE_OPEN,
    SND_SOC_DPCM_STATE_HW_PARAMS,
    SND_SOC_DPCM_STATE_PREPARE,
    SND_SOC_DPCM_STATE_START,
    SND_SOC_DPCM_STATE_STOP,
    SND_SOC_DPCM_STATE_PAUSED,
    SND_SOC_DPCM_STATE_SUSPEND,
    SND_SOC_DPCM_STATE_HW_FREE,
    SND_SOC_DPCM_STATE_CLOSE,
    } VX_SND_SOC_DPCM_STATE;

typedef enum vxSndSocBiasLevel
    {
    SND_SOC_BIAS_OFF = 0,
    SND_SOC_BIAS_STANDBY,
    SND_SOC_BIAS_PREPARE,
    SND_SOC_BIAS_ON,
    } VX_SND_SOC_BIAS_LEVEL;

enum snd_soc_dpcm_update
    {
    SND_SOC_DPCM_UPDATE_NO = 0,
    SND_SOC_DPCM_UPDATE_BE,
    SND_SOC_DPCM_UPDATE_FE,
    };

/*
 * Dynamic PCM trigger ordering. Triggering flexibility is required as some
 * DSPs require triggering before/after their CPU platform and DAIs.
 *
 * i.e. some clients may want to manually order this call in their PCM
 * trigger() whilst others will just use the regular core ordering.
 */

typedef enum snd_soc_dpcm_trigger
    {
    SND_SOC_DPCM_TRIGGER_PRE = 0,
    SND_SOC_DPCM_TRIGGER_POST,
    SND_SOC_DPCM_TRIGGER_BESPOKE,
    } VX_SND_SOC_DPCM_TRIGGER;

typedef struct vxSndSocDaiLinkComponent
    {
    const char *name;
    VXB_DEV_ID pDev;    //struct device_node *of_node;
    const char *dai_name;
    } VX_SND_SOC_DAI_LINK_COMPONENT;

typedef struct snd_soc_codec_conf
    {
    /*
     * specify device either by device name, or by
     * DT/OF node, but not both.
     */
    struct vxSndSocDaiLinkComponent dlc;

    /*
     * optional map of kcontrol, widget and path name prefixes that are
     * associated per device
     */
    const char *namePrefix;
    } VX_SND_SOC_CEDEC_CONF;

/* SoC card */
typedef struct vxSndSocCard
    {
    const char * name;
    const char * long_name;
    const char * driver_name;
    const char * components;
#ifdef CONFIG_DMI
    char dmi_longname[80];
#endif /* CONFIG_DMI */
    char topology_shortname[32];

    //struct device *dev;
    VXB_DEV_ID dev;
    SND_CARD * sndCard;

    SEM_ID cardMutex;   //mutex

    STATUS (*probe)(struct vxSndSocCard *card);
    STATUS (*late_probe)(struct vxSndSocCard *card);
    STATUS (*remove)(struct vxSndSocCard *card);

    /* the pre and post PM functions are used to do any PM work before and
     * after the codec and DAI's do any PM work. */
    STATUS (*suspend_pre)(struct vxSndSocCard *card);
    STATUS (*suspend_post)(struct vxSndSocCard *card);
    STATUS (*resume_pre)(struct vxSndSocCard *card);
    STATUS (*resume_post)(struct vxSndSocCard *card);

    int (*add_dai_link)(struct vxSndSocCard *,
                struct vxSndSocDaiLink *link);
    void (*remove_dai_link)(struct vxSndSocCard *,
                struct vxSndSocDaiLink *link);

    long pmdown_time;

    /* CPU <--> Codec DAI links  */
    struct vxSndSocDaiLink * dai_link;  /* predefined links only */
    int num_links;  /* predefined links only */

    DL_LIST rtd_list;
    int num_rtd;

    /* optional codec specific configuration */
    VX_SND_SOC_CEDEC_CONF *codec_conf;
    int num_configs;

    /*
     * optional auxiliary devices such as amplifiers or codecs with DAI
     * link unused
     */
    //  struct snd_soc_aux_dev *aux_dev;
    int num_aux_devs;
    //struct list_head aux_comp_list;

    VX_SND_CONTROL * controls;
    int num_controls;

    /* lists of probed devices belonging to this card */
    DL_LIST component_dev_list;
    DL_LIST list;

    uint32_t pop_time;

    /* bit field */
    unsigned int instantiated:1;
    unsigned int topology_shortname_created:1;
    unsigned int fully_routed:1;
    unsigned int disable_route_checks:1;
    unsigned int probed:1;
    unsigned int component_chaining:1;

    void *drvdata;
    } VX_SND_SOC_CARD;

/* SoC audio ops */
typedef struct vxSndSocOps
    {
    int  (*startup) (SND_PCM_SUBSTREAM *);
    void (*shutdown)(SND_PCM_SUBSTREAM *);
    int  (*hwParams)(SND_PCM_SUBSTREAM *, VX_SND_PCM_HW_PARAMS *);
    int  (*hwFree)  (SND_PCM_SUBSTREAM *);
    int  (*prepare) (SND_PCM_SUBSTREAM *);
    int  (*trigger) (SND_PCM_SUBSTREAM *, int);
    } VX_SND_SOC_OPS;

typedef struct vxSndSocDaiLink
    {
    /* config - must be set by machine driver */
    const char * name;           /* Codec name */
    const char * streamName;        /* Stream name */

    /*
     * You MAY specify the link's CPU-side device, either by device name,
     * or by DT/OF node, but not both. If this information is omitted,
     * the CPU-side DAI is matched using .cpu_dai_name only, which hence
     * must be globally unique. These fields are currently typically used
     * only for codec to codec links, or systems using device tree.
     */
    /*
     * You MAY specify the DAI name of the CPU DAI. If this information is
     * omitted, the CPU-side DAI is matched using .cpu_name/.cpu_of_node
     * only, which only works well when that device exposes a single DAI.
     */
    VX_SND_SOC_DAI_LINK_COMPONENT * cpus;
    unsigned int cpuNum;    //num_cpus

    /*
     * You MUST specify the link's codec, either by device name, or by
     * DT/OF node, but not both.
     */
    /* You MUST specify the DAI name within the codec */
    VX_SND_SOC_DAI_LINK_COMPONENT * codecs;
    unsigned int codecNum;  //num_codecs

    /*
     * You MAY specify the link's platform/PCM/DMA driver, either by
     * device name, or by DT/OF node, but not both. Some forms of link
     * do not need a platform. In such case, platforms are not mandatory.
     */
    VX_SND_SOC_DAI_LINK_COMPONENT * platforms;
    unsigned int platformNum;   //num_platforms

    int id; /* optional ID for machine driver link identification */

    const struct vxSndSocPcmStream *params;
    unsigned int num_params;

    unsigned int dai_fmt;           /* format to set on init */

    enum snd_soc_dpcm_trigger trigger[2]; /* trigger type for DPCM */

    /* codec/machine specific init - e.g. add machine controls */
    STATUS (*init)(struct vxSndSocPcmRuntime * rtd);

    /* codec/machine specific exit - dual of init() */
    void (*exit)(struct vxSndSocPcmRuntime * rtd);

    /* optional hw_params re-writing for BE and FE sync */
    STATUS (*be_hw_params_fixup)(struct vxSndSocPcmRuntime * rtd,
            VX_SND_PCM_HW_PARAMS *params);

    /* machine stream operations */
    const VX_SND_SOC_OPS *ops;
    //const struct snd_soc_compr_ops *compr_ops;

    /* Mark this pcm with non atomic ops */
    unsigned int nonatomic:1;

    /* For unidirectional dai links */
    unsigned int playbackOnly:1;
    unsigned int captureOnly:1;

    /* Keep DAI active over suspend */
    unsigned int ignore_suspend:1;

    /* Symmetry requirements */
    unsigned int symmetric_rate:1;
    unsigned int symmetric_channels:1;
    unsigned int symmetric_sample_bits:1;

    /* Do not create a PCM for this DAI link (Backend link) */
    unsigned int noPcm:1;

    /* This DAI link can route to other DAI links at runtime (Frontend)*/
    unsigned int dynamic:1;

    /* DPCM capture and Playback support */
    unsigned int dpcmCapture:1;
    unsigned int dpcmPlayback:1;

    /* DPCM used FE & BE merged format */
    unsigned int dpcm_merged_format:1;
    /* DPCM used FE & BE merged channel */
    unsigned int dpcm_merged_chan:1;
    /* DPCM used FE & BE merged rate */
    unsigned int dpcm_merged_rate:1;

    /* pmdown_time is ignored at stop */
    unsigned int ignore_pmdown_time:1;

    /* Do not create a PCM for this DAI link (Backend link) */
    unsigned int ignore:1;

    /* This flag will reorder stop sequence. By enabling this flag
     * DMA controller stop sequence will be invoked first followed by
     * CPU DAI driver stop sequence
     */
    unsigned int stopDmaFirst:1;
    } VX_SND_SOC_DAI_LINK;

/* SoC machine DAI configuration, glues a codec and cpu DAI together */
typedef struct vxSndSocPcmRuntime
    {
    DL_NODE node;

    VX_SND_SOC_CARD * card;
    VX_SND_SOC_DAI_LINK * daiLink;
    VX_SND_PCM_STREAM_OPS ops;

    unsigned int params_select; /* currently selected param for dai link */

    /* Dynamic PCM BE runtime data */

    struct vxSndSocPcmRuntime * pBe[2];
    VX_SND_SOC_DPCM_STATE dpcmState[SNDRV_PCM_STREAM_MAX];
    VX_SND_PCM_HW_PARAMS  dpcmUserParams;

    /* runtime devices */
    VX_SND_PCM *pcm;

    /*
     * dais = cpu_dai + codec_dai
     * see
     *  soc_new_pcm_runtime()
     *  SOC_RTD_TO_CPU()
     *  SOC_RTD_TO_CODEC()
     */
    struct vxSndSocDai **dais;
    unsigned int codecNum;
    unsigned int cpuNum;

    unsigned int num; /* 0-based and monotonic increasing */

    /* function mark */
    SND_PCM_SUBSTREAM *mark_startup;
    SND_PCM_SUBSTREAM *mark_hw_params;
    SND_PCM_SUBSTREAM *mark_trigger;

    /* bit field */
    unsigned int popWait:1;

    int componentNum;
    struct vxSndSocComponent *components[]; /* CPU/Codec/Platform */
    } VX_SND_SOC_PCM_RUNTIME;

typedef struct vxSndSocDaiOps
    {
    /*
     * DAI clocking configuration, all optional.
     * Called by soc_card drivers, normally in their hw_params.
     */
    int (*set_sysclk)(struct vxSndSocDai *dai,
        int clk_id, unsigned int freq, int dir);
    int (*set_pll)(struct vxSndSocDai *dai, int pll_id, int source,
        unsigned int freq_in, unsigned int freq_out);
    int (*set_clkdiv)(struct vxSndSocDai *dai, int div_id, int div);
    int (*set_bclk_ratio)(struct vxSndSocDai *dai, unsigned int ratio);

    /*
     * DAI format configuration
     * Called by soc_card drivers, normally in their hw_params.
     */
    STATUS (*setFmt)(struct vxSndSocDai *dai, unsigned int fmt);
    STATUS (*xlate_tdm_slot_mask)(unsigned int slots,
        unsigned int *tx_mask, unsigned int *rx_mask);
    STATUS (*set_tdm_slot)(struct vxSndSocDai *dai,
        unsigned int tx_mask, unsigned int rx_mask,
        int slots, int slot_width);
    STATUS (*set_channel_map)(struct vxSndSocDai *dai,
        unsigned int tx_num, unsigned int *tx_slot,
        unsigned int rx_num, unsigned int *rx_slot);
    int (*get_channel_map)(struct vxSndSocDai *dai,
            unsigned int *tx_num, unsigned int *tx_slot,
            unsigned int *rx_num, unsigned int *rx_slot);
    STATUS (*set_tristate)(struct vxSndSocDai *dai, int tristate);

    int (*set_sdw_stream)(struct vxSndSocDai *dai,
            void *stream, int direction);
    void *(*get_sdw_stream)(struct vxSndSocDai *dai, int direction);

    /*
     * DAI digital mute - optional.
     * Called by soc-core to minimise any pops.
     */
    STATUS (*muteStream)(struct vxSndSocDai *dai, int mute, int stream);

    /*
     * ALSA PCM audio operations - all optional.
     * Called by soc-core during audio PCM operations.
     */
    STATUS (*startup)(SND_PCM_SUBSTREAM *,
        struct vxSndSocDai *);
    void (*shutdown)(SND_PCM_SUBSTREAM *,
        struct vxSndSocDai *);
    STATUS (*hwParams)(SND_PCM_SUBSTREAM *,
        VX_SND_PCM_HW_PARAMS *, struct vxSndSocDai *);
    STATUS (*hwFree)(SND_PCM_SUBSTREAM *,
        struct vxSndSocDai *);
    STATUS (*prepare)(SND_PCM_SUBSTREAM *,
        struct vxSndSocDai *);
    /*
     * NOTE: Commands passed to the trigger function are not necessarily
     * compatible with the current state of the dai. For example this
     * sequence of commands is possible: START STOP STOP.
     * So do not unconditionally use refcounting functions in the trigger
     * function, e.g. clk_enable/disable.
     */
    STATUS (*trigger)(SND_PCM_SUBSTREAM *, int,
        struct vxSndSocDai *);
    int (*bespoke_trigger)(SND_PCM_SUBSTREAM *, int,
        struct vxSndSocDai *);
    /*
     * For hardware based FIFO caused delay reporting.
     * Optional.
     */
    VX_SND_PCM (*delay)(SND_PCM_SUBSTREAM *,
        struct vxSndSocDai *);

    /*
     * Format list for auto selection.
     * Format will be increased if priority format was
     * not selected.
     * see
     *  snd_soc_dai_get_fmt()
     */
    uint64_t *auto_selectable_formats;
    int num_auto_selectable_formats;

    /* bit field */
    unsigned int noCaptureMute:1;
    } VX_SND_SOC_DAI_OPS;

/* SoC PCM stream information */
typedef struct vxSndSocPcmStream
    {
    const char *streamName;
    uint64_t formats;           /* SNDRV_PCM_FMTBIT_* */
    uint32_t rates;     /* SNDRV_PCM_RATE_* */
    uint32_t rateMin;       /* min rate */
    uint32_t rateMax;       /* max rate */
    uint32_t channelsMin;   /* min channels */
    uint32_t channelsMax;   /* max channels */
    uint32_t sigBits;       /* number of bits of content */
    } VX_SND_SOC_PCM_STREAM;

/*
 * Digital Audio Interface Driver.
 *
 * Describes the Digital Audio Interface in terms of its ALSA, DAI and AC97
 * operations and capabilities. Codec and platform drivers will register this
 * structure for every DAI they have.
 *
 * This structure covers the clocking, formating and ALSA operations for each
 * interface.
 */
typedef struct vxSndSocDaiDriver
    {
    /* DAI description */
    const char *name;
    uint32_t id;

    /* DAI driver callbacks */
    STATUS (*probe)(struct vxSndSocDai *dai);
    STATUS (*remove)(struct vxSndSocDai *dai);

    /* Optional Callback used at pcm creation*/
    STATUS (*pcm_new)(VX_SND_SOC_PCM_RUNTIME *rtd,
               struct vxSndSocDai *dai);

    /* ops */
    const struct vxSndSocDaiOps *ops;

    /* DAI capabilities */
    struct vxSndSocPcmStream capture;
    struct vxSndSocPcmStream playback;
    uint32_t symmetric_rate:1;
    uint32_t symmetric_channels:1;
    uint32_t symmetric_sample_bits:1;

    /* probe ordering - for components with runtime dependencies */
    int probe_order;
    int remove_order;
    } VX_SND_SOC_DAI_DRIVER;

/*
 * Digital Audio Interface runtime data.
 *
 * Holds runtime data for a DAI.
 */
//snd_soc_dai
typedef struct vxSndSocDai
    {
    DL_NODE         node; /* node should be the first member */
    const char *name;
    int id;
    VXB_DEV_ID  pDev;

    /* driver ops */
    VX_SND_SOC_DAI_DRIVER *driver;

    /* DAI runtime info */
    uint32_t streamActiveCnt[SNDRV_PCM_STREAM_MAX]; /* usage count */

    /* DAI DMA data */
    void *playback_dma_data;
    void *capture_dma_data;

    /* Symmetry data - only valid if symmetry is being enforced */
    uint32_t rate;
    uint32_t channels;
    uint32_t sampleBits;

    /* parent platform/codec */
    struct vxSndSocComponent *component;

    /* CODEC TDM slot masks and params (for fixup) */
    uint32_t tdmTxMask;
    uint32_t tdmRxMask;

    /* bit field */
    uint32_t probed:1;
    } VX_SND_SOC_DAI;


typedef struct vxSndSocComponentDriver
    {
    const char *name;

    /* Default control and setup, added after probe() is run */
    const VX_SND_CONTROL * controls;
    uint32_t num_controls;

    STATUS (*probe)(struct vxSndSocComponent *component);
    void (*remove)(struct vxSndSocComponent *component);

    uint32_t (*read)(struct vxSndSocComponent *component,
                 uint32_t reg);
    STATUS (*write)(struct vxSndSocComponent *component,
             uint32_t reg, uint32_t val);

    /* pcm creation and destruction */
    STATUS (*pcmConstruct)(struct vxSndSocComponent *component,
                 VX_SND_SOC_PCM_RUNTIME *rtd);
    void (*pcm_destruct)(struct vxSndSocComponent *component,
                 VX_SND_PCM *pcm);

    /* component wide operations */
    STATUS (*set_sysclk)(struct vxSndSocComponent *component,
              int clk_id, int source, uint32_t freq, int dir);
    STATUS (*set_pll)(struct vxSndSocComponent *component, int pll_id,
               int source, uint32_t freq_in, uint32_t freq_out);

    /* DT */
    STATUS (*of_xlate_dai_name)(struct vxSndSocComponent *component,
                 const struct of_phandle_args *args,
                 const char **dai_name);
    STATUS (*of_xlate_dai_id)(struct vxSndSocComponent *comment,
                   struct device_node *endpoint);

    STATUS (*stream_event)(struct vxSndSocComponent *component, int event);
    STATUS (*set_bias_level)(struct vxSndSocComponent *component,
                  enum snd_soc_bias_level level);

    STATUS (*open)(struct vxSndSocComponent *component,
            SND_PCM_SUBSTREAM *substream);
    STATUS (*close)(struct vxSndSocComponent *component,
             SND_PCM_SUBSTREAM *substream);
    STATUS (*ioctl)(struct vxSndSocComponent *component,
             SND_PCM_SUBSTREAM *substream,
             uint32_t cmd, void *arg);
    STATUS (*hwParams)(struct vxSndSocComponent *component,
             SND_PCM_SUBSTREAM *substream,
             VX_SND_PCM_HW_PARAMS *params);
    STATUS (*hwFree)(struct vxSndSocComponent *component,
               SND_PCM_SUBSTREAM *substream);
    STATUS (*prepare)(struct vxSndSocComponent *component,
               SND_PCM_SUBSTREAM *substream);
    STATUS (*trigger)(struct vxSndSocComponent *component,
               SND_PCM_SUBSTREAM *substream, int cmd);
    STATUS (*syncStop)(struct vxSndSocComponent *component,
             SND_PCM_SUBSTREAM *substream);
    SND_FRAMES_T (*pointer)(struct vxSndSocComponent *component,
                     SND_PCM_SUBSTREAM *substream);

    STATUS (*copy)(struct vxSndSocComponent *component,
             SND_PCM_SUBSTREAM *substream, int channel,
             unsigned long pos, void *buf,
             unsigned long bytes);

    STATUS (*mmap)(struct vxSndSocComponent *component,
            SND_PCM_SUBSTREAM *substream);
    STATUS (*ack)(struct vxSndSocComponent *component,
           SND_PCM_SUBSTREAM *substream);

    /* probe ordering - for components with runtime dependencies */
    int probe_order;
    int remove_order;

    /*
     * signal if the module handling the component should not be removed
     * if a pcm is open. Setting this would prevent the module
     * refcount being incremented in probe() but allow it be incremented
     * when a pcm is opened and decremented when it is closed.
     */
    uint32_t module_get_upon_open:1;

    /* bits */
    uint32_t idle_bias_on:1;
    uint32_t suspend_bias_off:1;
    uint32_t use_pmdown_time:1; /* care pmdown_time at stop */
    uint32_t endianness:1;

    /* this component uses topology and ignore machine driver FEs */
    const char *ignore_machine;
    const char *topology_name_prefix;
    int (*be_hw_params_fixup)(VX_SND_SOC_PCM_RUNTIME *rtd,
                  VX_SND_PCM_HW_PARAMS *params);
    BOOL use_dai_pcm_id;    /* use DAI link PCM ID as PCM device number */
    int be_pcm_base;    /* base device ID for all BE PCMs */
    } VX_SND_SOC_CMPT_DRV;

//snd_soc_component
typedef struct vxSndSocComponent
    {
    DL_NODE         node; /* node should be the first member, component_list */
    char *name;
    int id;
    const char *namePrefix;
    VXB_DEV_ID  pDev;
    VX_SND_SOC_CARD *card;

    uint32_t activeCnt;

    uint32_t suspended:1; /* is in suspend PM state */

    DL_NODE cardNode; /* not need to be first */

    const VX_SND_SOC_CMPT_DRV *driver;

    DL_LIST dai_list;//struct list_head dai_list;
    int numDai;

    VXB_REG_MAP * regmap;
    int val_bytes;

    SEM_ID  ioMutex;

    /* attached dynamic objects */
    //struct list_head dobj_list;

    /*
     * DO NOT use any of the fields below in drivers, they are temporary and
     * are going to be removed again soon. If you use them in driver code
     * the driver will be marked as BROKEN when these fields are removed.
     */

    STATUS (*init)(struct vxSndSocComponent *component);

    void *mark_pm;

    } VX_SND_SOC_COMPONENT;

STATUS vxSndSocPcmCreate (VX_SND_SOC_PCM_RUNTIME * runtime, int idx);
STATUS vxSndSocDaiStartup (VX_SND_SOC_DAI * dai, SND_PCM_SUBSTREAM * substream);
void vxSndSocDaiShutdown (VX_SND_SOC_DAI * dai, SND_PCM_SUBSTREAM * substream);
void vxSndSocRuntimeHwparamsSet (SND_PCM_SUBSTREAM * substream,
                                 const VX_SND_PCM_HARDWARE * hw);
                                 uint32_t vxSndSocDaiActiveAllCntGet
                                 (VX_SND_SOC_DAI * dai);
uint32_t vxSndSocDaiActiveStreamCntGet (VX_SND_SOC_DAI * dai,
                                        STREAM_DIRECT direct);
uint32_t vxSndSocComponentActiveCntGet (VX_SND_SOC_COMPONENT * component);
STATUS sndSocLinkBeHwParamsFixup (VX_SND_SOC_PCM_RUNTIME * runtime,
                                  VX_SND_PCM_HW_PARAMS * params);
STATUS vxSndSocDaiDigitalMute (VX_SND_SOC_DAI * dai, BOOL mute,
                               STREAM_DIRECT direct);
void vxSndSocDaiHwFree (VX_SND_SOC_DAI * dai, SND_PCM_SUBSTREAM * substream);
STATUS vxSocPcmComponentAck (SND_PCM_SUBSTREAM * substream);
STATUS vxSndSocComponentCtrlsAdd (VX_SND_SOC_COMPONENT * component,
                                  VX_SND_CONTROL * ctrlList, uint32_t ctrlNum);
STATUS vxSndSocDaiCtrlsAdd (VX_SND_SOC_DAI * dai, VX_SND_CONTROL * ctrlList,
                            uint32_t ctrlNum);
STATUS vxSndSocCardCtrlsAdd (VX_SND_SOC_CARD * socCard,
                             VX_SND_CONTROL * ctrlList, int ctrlNum);
uint32_t vxSndSocComponentRead (VX_SND_SOC_COMPONENT * component,
                                uint32_t reg);
uint32_t vxSndSocComponentReadUnlock (VX_SND_SOC_COMPONENT * component,
                                      uint32_t reg);
STATUS vxSndSocComponentWrite (VX_SND_SOC_COMPONENT * component,
                               uint32_t reg, uint32_t val);

STATUS vxSndSocComponentWriteUnlock (VX_SND_SOC_COMPONENT * component,
                                     uint32_t reg, uint32_t val);
STATUS vxSndSocComponentUpdate (VX_SND_SOC_COMPONENT * component,
                                uint32_t reg, uint32_t mask, uint32_t data);
VX_SND_SOC_PCM_RUNTIME * sndSocCardrtdFindByName (VX_SND_SOC_CARD * socCard,
                                                  const char * rtdName);
STATUS sndSocBeConnectByName (VX_SND_SOC_CARD * socCard,
                              const char * nameFront,
                              const char * nameBack,
                              STREAM_DIRECT direct);
STATUS vxSndCpntRegister (VXB_DEV_ID pDev,
                          const VX_SND_SOC_CMPT_DRV * cpntDriver,
                          VX_SND_SOC_DAI_DRIVER * daiDrv, int32_t daiNum);
BOOL snd_soc_component_is_dummy (VX_SND_SOC_COMPONENT * component);

STATUS vxSndCpntRegister (VXB_DEV_ID pDev,
                          const VX_SND_SOC_CMPT_DRV * cpntDriver,
                          VX_SND_SOC_DAI_DRIVER * daiDrv, int32_t daiNum);
STATUS vxSndCardRegister (VX_SND_SOC_CARD * pCard);
STATUS snd_soc_unregister_card (VX_SND_SOC_CARD * card);
STATUS snd_soc_add_component (VX_SND_SOC_COMPONENT * component,
                              VX_SND_SOC_DAI_DRIVER * dai_drv,
                              int32_t num_dai);
VX_SND_SOC_DAI * snd_soc_find_dai (const VX_SND_SOC_DAI_LINK_COMPONENT * dlc);
int32_t vxSndSocDaiFmtSet (VX_SND_SOC_DAI * dai, uint32_t fmt);
STATUS sndSocGetDaiName (VXB_DEV_ID pDev, uint32_t daiIdx,
                         const char ** dai_name);
STATUS snd_soc_dai_set_tdm_slot (VX_SND_SOC_DAI * dai, uint32_t tx_mask,
                                 uint32_t rx_mask, int32_t slots,
                                 int32_t slot_width);
STATUS snd_soc_dai_set_tristate (VX_SND_SOC_DAI * dai, int tristate);
void snd_soc_unregister_component (VXB_DEV_ID pDev);

#endif

#if __cplusplus
}
#endif /* __cplusplus  */

#endif /* __INCvxSndSoch */
