/* vxbFdtAboxGic.h - Samsung Abox GIC driver header file */

/*
 * The source code in this file refers to or is derived from GPL-licensed
 * source code. It is strictly proprietary and must NOT be distributed
 * externally under any circumstances.
 */

/*
DESCRIPTION

*/

#ifndef __INCvxbFdtAboxGich
#define __INCvxbFdtAboxGich

#include <vxWorks.h>
#include <hwif/vxbus/vxbLib.h>
#include <hwif/util/vxbResourceLib.h>
#include <aboxIpc.h>
#include <vxbFdtAboxCtr.h>

#ifdef __cplusplus
extern "C" {
#endif

/* DTS value defines */

#define GICD_SET_ALL_WITHOUT_DOMAINS    0
#define GICD_SET_ALL_WITH_DOMAINS       1

/* DTS value defines end */

#define ABOX_GIC_DRV_NAME       "aboxGic"

#define ABOX_MAIN_CPU_SPI_IDX   0   /* choose the first SPI(55) to main CPU */

#define ABOX_GIC_IRQ_TASK_NAME  "aboxGicIrqTask"
#define ABOX_GIC_IRQ_TASK_PRI   75
#define ABOX_GIC_IRQ_TASK_STACK (32 * 1024)

#define ABOX_GIC_STR_MAX        64

//FIRST_OF_SPI
#define ABOX_GIC_SPI_OFS        32

#define NUM_OF_SGI              16
#define ABOX_GIC_IRQ_QUEUE_SIZE 100

/* registers */

#define GIC_CPU_CTRL            0x00
#define GIC_CPU_PRIMASK         0x04
#define GIC_CPU_BINPOINT        0x08
#define GIC_CPU_INTACK          0x0C
#define GIC_CPU_EOI             0x10
#define GIC_CPU_RUNNINGPRI      0x14
#define GIC_CPU_HIGHPRI         0x18
#define GIC_CPU_ALIAS_BINPOINT  0x1C
#define GIC_CPU_ACTIVEPRIO      0xD0
#define GIC_CPU_IDENT           0xFC
#define GIC_CPU_DEACTIVATE      0x1000

#define GICC_IAR_INT_ID_MASK    0x3FF

#define GIC_DIST_CTRL               0x000
#define GIC_DIST_CTR                0x004
#define GIC_DIST_IIDR               0x008
#define GIC_DIST_IGROUP             0x080
#define GIC_DIST_ENABLE_SET         0x100
#define GIC_DIST_ENABLE_CLEAR       0x180
#define GIC_DIST_PENDING_SET        0x200
#define GIC_DIST_PENDING_CLEAR      0x280
#define GIC_DIST_ACTIVE_SET         0x300
#define GIC_DIST_ACTIVE_CLEAR       0x380
#define GIC_DIST_PRI                0x400
#define GIC_DIST_TARGET             0x800
#define GIC_DIST_CONFIG             0xC00
#define GIC_DIST_SOFTINT            0xF00
#define GIC_DIST_SGI_PENDING_CLEAR  0xF10
#define GIC_DIST_SGI_PENDING_SET    0xF20


#define IS_SPI_OF_ABOX_RDMA(id)         \
    (RDMA_FIRST_BUF_EMPTY_V920_IRQ <= id && id <= RDMA_LAST_BUF_EMPTY_V920_IRQ)

#define IS_SPI_OF_ABOX_WDMA(id)         \
    (WDMA_FIRST_BUF_FULL_V920_IRQ <= id && id <= WDMA_LAST_BUF_FULL_V920_IRQ)

#define IS_SPI_OF_ABOX_RX_MAILBOX(id)   \
    (                                   \
    MBOX4_INTR0_V920_IRQ == id          \
    || MBOX4_INTR1_V920_IRQ == id       \
    || MBOX4_INTR2_V920_IRQ == id       \
    || MBOX4_INTR3_V920_IRQ == id       \
    || MBOX5_INTR0_V920_IRQ == id       \
    || MBOX5_INTR1_V920_IRQ == id       \
    || MBOX5_INTR2_V920_IRQ == id       \
    || MBOX5_INTR3_V920_IRQ == id       \
    || MBOX6_INTR0_V920_IRQ == id       \
    || MBOX6_INTR1_V920_IRQ == id       \
    || MBOX6_INTR2_V920_IRQ == id       \
    || MBOX6_INTR3_V920_IRQ == id       \
    || MBOX7_INTR0_V920_IRQ == id       \
    || MBOX7_INTR1_V920_IRQ == id       \
    || MBOX7_INTR2_V920_IRQ == id       \
    || MBOX7_INTR3_V920_IRQ == id       \
    )

#define IS_SPI_OF_ABOX_DMA(id)          \
    (IS_SPI_OF_ABOX_RDMA(id) || IS_SPI_OF_ABOX_WDMA(id))


typedef enum gicdIcfgrType
    {
    GICD_LEVEL_SENSITIVE,
    GICD_EDGE_TRIGGER
    } GICD_ICFGR_TYPE;

typedef enum gicdIcfgrModel
    {
    GICD_N_N_MODEL,
    GICD_1_N_MODEL
    } GICD_ICFGR_MODEL;

typedef enum gicCpuId
    {
    ABOX_GIC_CPU0,
    ABOX_GIC_CPU1,
    ABOX_GIC_CPU2,
    ABOX_GIC_CPU3,
    ABOX_GIC_CPU4,
    ABOX_GIC_CPU5,
    ABOX_GIC_CPU6,
    ABOX_GIC_CPU7,
    ABOX_GIC_MAX,
    } GIC_CPU_ID;

typedef enum aboxV920GicIrq
    {
    /* SGI Interrupts Numbers */
    SGI0_V920_IRQ                  = 0,
    SGI1_V920_IRQ                  = 1,
    SGI2_V920_IRQ                  = 2,
    SGI3_V920_IRQ                  = 3,
    SGI4_V920_IRQ                  = 4,
    SGI5_V920_IRQ                  = 5,
    SGI6_V920_IRQ                  = 6,
    SGI7_V920_IRQ                  = 7,
    SGI8_V920_IRQ                  = 8,
    SGI9_V920_IRQ                  = 9,
    SGI10_V920_IRQ                 = 10,
    SGI11_V920_IRQ                 = 11,
    SGI12_V920_IRQ                 = 12,
    SGI13_V920_IRQ                 = 13,
    SGI14_V920_IRQ                 = 14,
    SGI15_V920_IRQ                 = 15,

    /* SPI Interrupts Numbers */
    RDMA23_CRC_DONE_V920_IRQ       = 0 + 32,
    MIXP_CRC_DONE_V920_IRQ         = 1 + 32,
    UAIFS_CRC_DONE_V920_IRQ        = 2 + 32,
    RESERVED_3_V920_IRQ            = 3 + 32,
    MIXP_COMP_ERR_V920_IRQ         = 4 + 32,
    RESERVED_5_V920_IRQ            = 5 + 32,

    RDMA_FIRST_BUF_EMPTY_V920_IRQ  = 6 + 32,
    RDMA0_BUF_EMPTY_V920_IRQ       = 6 + 32,
    RDMA1_BUF_EMPTY_V920_IRQ       = 7 + 32,
    RDMA2_BUF_EMPTY_V920_IRQ       = 8 + 32,
    RDMA3_BUF_EMPTY_V920_IRQ       = 9 + 32,
    RDMA4_BUF_EMPTY_V920_IRQ       = 10 + 32,
    RDMA5_BUF_EMPTY_V920_IRQ       = 11 + 32,
    RDMA6_BUF_EMPTY_V920_IRQ       = 12 + 32,
    RDMA7_BUF_EMPTY_V920_IRQ       = 13 + 32,
    RDMA8_BUF_EMPTY_V920_IRQ       = 14 + 32,
    RDMA9_BUF_EMPTY_V920_IRQ       = 15 + 32,
    RDMA10_BUF_EMPTY_V920_IRQ      = 16 + 32,
    RDMA11_BUF_EMPTY_V920_IRQ      = 17 + 32,
    RDMA12_BUF_EMPTY_V920_IRQ      = 18 + 32,
    RDMA13_BUF_EMPTY_V920_IRQ      = 19 + 32,
    RDMA14_BUF_EMPTY_V920_IRQ      = 20 + 32,
    RDMA15_BUF_EMPTY_V920_IRQ      = 21 + 32,
    RDMA16_BUF_EMPTY_V920_IRQ      = 22 + 32,
    RDMA17_BUF_EMPTY_V920_IRQ      = 23 + 32,
    RDMA18_BUF_EMPTY_V920_IRQ      = 24 + 32,
    RDMA19_BUF_EMPTY_V920_IRQ      = 25 + 32,
    RDMA20_BUF_EMPTY_V920_IRQ      = 26 + 32,
    RDMA21_BUF_EMPTY_V920_IRQ      = 27 + 32,
    RDMA22_BUF_EMPTY_V920_IRQ      = 28 + 32,
    RDMA23_BUF_EMPTY_V920_IRQ      = 29 + 32,
    RDMA_LAST_BUF_EMPTY_V920_IRQ   = RDMA23_BUF_EMPTY_V920_IRQ,

    RDMA_FIRST_FADE_DONE_V920_IRQ  = 30 + 32,
    RDMA0_FADE_DONE_V920_IRQ       = 30 + 32,
    RDMA1_FADE_DONE_V920_IRQ       = 31 + 32,
    RDMA2_FADE_DONE_V920_IRQ       = 32 + 32,
    RDMA3_FADE_DONE_V920_IRQ       = 33 + 32,
    RDMA4_FADE_DONE_V920_IRQ       = 34 + 32,
    RDMA5_FADE_DONE_V920_IRQ       = 35 + 32,
    RDMA6_FADE_DONE_V920_IRQ       = 36 + 32,
    RDMA7_FADE_DONE_V920_IRQ       = 37 + 32,
    RDMA8_FADE_DONE_V920_IRQ       = 38 + 32,
    RDMA9_FADE_DONE_V920_IRQ       = 39 + 32,
    RDMA10_FADE_DONE_V920_IRQ      = 40 + 32,
    RDMA11_FADE_DONE_V920_IRQ      = 41 + 32,
    RDMA12_FADE_DONE_V920_IRQ      = 42 + 32,
    RDMA13_FADE_DONE_V920_IRQ      = 43 + 32,
    RDMA14_FADE_DONE_V920_IRQ      = 44 + 32,
    RDMA15_FADE_DONE_V920_IRQ      = 45 + 32,
    RDMA16_FADE_DONE_V920_IRQ      = 46 + 32,
    RDMA17_FADE_DONE_V920_IRQ      = 47 + 32,
    RDMA18_FADE_DONE_V920_IRQ      = 48 + 32,
    RDMA19_FADE_DONE_V920_IRQ      = 49 + 32,
    RDMA20_FADE_DONE_V920_IRQ      = 50 + 32,
    RDMA21_FADE_DONE_V920_IRQ      = 51 + 32,
    RDMA22_FADE_DONE_V920_IRQ      = 52 + 32,
    RDMA23_FADE_DONE_V920_IRQ      = 53 + 32,
    RDMA_LAST_FADE_DONE_V920_IRQ   = 53 + 32,

    WDMA_FIRST_BUF_FULL_V920_IRQ   = 54 + 32,
    WDMA0_BUF_FULL_V920_IRQ        = 54 + 32,
    WDMA1_BUF_FULL_V920_IRQ        = 55 + 32,
    WDMA2_BUF_FULL_V920_IRQ        = 56 + 32,
    WDMA3_BUF_FULL_V920_IRQ        = 57 + 32,
    WDMA4_BUF_FULL_V920_IRQ        = 58 + 32,
    WDMA5_BUF_FULL_V920_IRQ        = 59 + 32,
    WDMA6_BUF_FULL_V920_IRQ        = 60 + 32,
    WDMA7_BUF_FULL_V920_IRQ        = 61 + 32,
    WDMA8_BUF_FULL_V920_IRQ        = 62 + 32,
    WDMA9_BUF_FULL_V920_IRQ        = 63 + 32,
    WDMA10_BUF_FULL_V920_IRQ       = 64 + 32,
    WDMA11_BUF_FULL_V920_IRQ       = 65 + 32,
    WDMA12_BUF_FULL_V920_IRQ       = 66 + 32,
    WDMA13_BUF_FULL_V920_IRQ       = 67 + 32,
    WDMA14_BUF_FULL_V920_IRQ       = 68 + 32,
    WDMA15_BUF_FULL_V920_IRQ       = 69 + 32,
    WDMA16_BUF_FULL_V920_IRQ       = 70 + 32,
    WDMA17_BUF_FULL_V920_IRQ       = 71 + 32,
    WDMA18_BUF_FULL_V920_IRQ       = 72 + 32,
    WDMA19_BUF_FULL_V920_IRQ       = 73 + 32,
    WDMA20_BUF_FULL_V920_IRQ       = 74 + 32,
    WDMA21_BUF_FULL_V920_IRQ       = 75 + 32,
    WDMA_LAST_BUF_FULL_V920_IRQ    = 75 + 32,

    WDMA0_FIRST_FADE_DONE_V920_IRQ = 76 + 32,
    WDMA0_FADE_DONE_V920_IRQ       = 76 + 32,
    WDMA1_FADE_DONE_V920_IRQ       = 77 + 32,
    WDMA2_FADE_DONE_V920_IRQ       = 78 + 32,
    WDMA3_FADE_DONE_V920_IRQ       = 79 + 32,
    WDMA4_FADE_DONE_V920_IRQ       = 80 + 32,
    WDMA5_FADE_DONE_V920_IRQ       = 81 + 32,
    WDMA6_FADE_DONE_V920_IRQ       = 82 + 32,
    WDMA7_FADE_DONE_V920_IRQ       = 83 + 32,
    WDMA8_FADE_DONE_V920_IRQ       = 84 + 32,
    WDMA9_FADE_DONE_V920_IRQ       = 85 + 32,
    WDMA10_FADE_DONE_V920_IRQ      = 86 + 32,
    WDMA11_FADE_DONE_V920_IRQ      = 87 + 32,
    WDMA12_FADE_DONE_V920_IRQ      = 88 + 32,
    WDMA13_FADE_DONE_V920_IRQ      = 89 + 32,
    WDMA14_FADE_DONE_V920_IRQ      = 90 + 32,
    WDMA15_FADE_DONE_V920_IRQ      = 91 + 32,
    WDMA16_FADE_DONE_V920_IRQ      = 92 + 32,
    WDMA17_FADE_DONE_V920_IRQ      = 93 + 32,
    WDMA18_FADE_DONE_V920_IRQ      = 94 + 32,
    WDMA19_FADE_DONE_V920_IRQ      = 95 + 32,
    WDMA20_FADE_DONE_V920_IRQ      = 96 + 32,
    WDMA21_FADE_DONE_V920_IRQ      = 97 + 32,
    WDMA_LAST_FADE_DONE_V920_IRQ   = 97 + 32,

    RDMA0_ERR_V920_IRQ             = 98 + 32,
    RDMA1_ERR_V920_IRQ             = 99 + 32,
    RDMA2_ERR_V920_IRQ             = 100 + 32,
    RDMA3_ERR_V920_IRQ             = 101 + 32,
    RDMA4_ERR_V920_IRQ             = 102 + 32,
    RDMA5_ERR_V920_IRQ             = 103 + 32,
    RDMA6_ERR_V920_IRQ             = 104 + 32,
    RDMA7_ERR_V920_IRQ             = 105 + 32,
    RDMA8_ERR_V920_IRQ             = 106 + 32,
    RDMA9_ERR_V920_IRQ             = 107 + 32,
    RDMA10_ERR_V920_IRQ            = 108 + 32,
    RDMA11_ERR_V920_IRQ            = 109 + 32,
    RDMA12_ERR_V920_IRQ            = 110 + 32,
    RDMA13_ERR_V920_IRQ            = 111 + 32,
    RDMA14_ERR_V920_IRQ            = 112 + 32,
    RDMA15_ERR_V920_IRQ            = 113 + 32,
    RDMA16_ERR_V920_IRQ            = 114 + 32,
    RDMA17_ERR_V920_IRQ            = 115 + 32,
    RDMA18_ERR_V920_IRQ            = 116 + 32,
    RDMA19_ERR_V920_IRQ            = 117 + 32,
    RDMA20_ERR_V920_IRQ            = 118 + 32,
    RDMA21_ERR_V920_IRQ            = 119 + 32,
    RDMA22_ERR_V920_IRQ            = 120 + 32,
    RDMA23_ERR_V920_IRQ            = 121 + 32,

    WDMA0_ERR_V920_IRQ             = 122 + 32,
    WDMA1_ERR_V920_IRQ             = 123 + 32,
    WDMA2_ERR_V920_IRQ             = 124 + 32,
    WDMA3_ERR_V920_IRQ             = 125 + 32,
    WDMA4_ERR_V920_IRQ             = 126 + 32,
    WDMA5_ERR_V920_IRQ             = 127 + 32,
    WDMA6_ERR_V920_IRQ             = 128 + 32,
    WDMA7_ERR_V920_IRQ             = 129 + 32,
    WDMA8_ERR_V920_IRQ             = 130 + 32,
    WDMA9_ERR_V920_IRQ             = 131 + 32,
    WDMA10_ERR_V920_IRQ            = 132 + 32,
    WDMA11_ERR_V920_IRQ            = 133 + 32,
    WDMA12_ERR_V920_IRQ            = 134 + 32,
    WDMA13_ERR_V920_IRQ            = 135 + 32,
    WDMA14_ERR_V920_IRQ            = 136 + 32,
    WDMA15_ERR_V920_IRQ            = 137 + 32,
    WDMA16_ERR_V920_IRQ            = 138 + 32,
    WDMA17_ERR_V920_IRQ            = 139 + 32,
    WDMA18_ERR_V920_IRQ            = 140 + 32,
    WDMA19_ERR_V920_IRQ            = 141 + 32,
    WDMA20_ERR_V920_IRQ            = 142 + 32,
    WDMA21_ERR_V920_IRQ            = 143 + 32,

    SPUS_SIFM_FADE_DONE_V920_IRQ   = 144 + 32,

    AUDEN_MIXP0_SYNC_V920_IRQ      = 145 + 32,
    AUDEN_MIXP1_SYNC_V920_IRQ      = 146 + 32,
    AUDEN_MIXP2_SYNC_V920_IRQ      = 147 + 32,

    AUDEN_RDMA0_BUF_EMPTY_V920_IRQ = 148 + 32,
    AUDEN_RDMA1_BUF_EMPTY_V920_IRQ = 149 + 32,
    AUDEN_RDMA2_BUF_EMPTY_V920_IRQ = 150 + 32,
    AUDEN_RDMA3_BUF_EMPTY_V920_IRQ = 151 + 32,
    AUDEN_RDMA4_BUF_EMPTY_V920_IRQ = 152 + 32,
    AUDEN_RDMA5_BUF_EMPTY_V920_IRQ = 153 + 32,
    AUDEN_RDMA6_BUF_EMPTY_V920_IRQ = 154 + 32,
    AUDEN_RDMA7_BUF_EMPTY_V920_IRQ = 155 + 32,
    AUDEN_RDMA8_BUF_EMPTY_V920_IRQ = 156 + 32,
    AUDEN_RDMA9_BUF_EMPTY_V920_IRQ = 157 + 32,
    AUDEN_RDMA10_BUF_EMPTY_V920_IRQ = 158 + 32,
    AUDEN_RDMA11_BUF_EMPTY_V920_IRQ = 159 + 32,

    AUDEN_RDMA0_FADE_DONE_V920_IRQ = 160 + 32,
    AUDEN_RDMA1_FADE_DONE_V920_IRQ = 161 + 32,
    AUDEN_RDMA2_FADE_DONE_V920_IRQ = 162 + 32,
    AUDEN_RDMA3_FADE_DONE_V920_IRQ = 163 + 32,
    AUDEN_RDMA4_FADE_DONE_V920_IRQ = 164 + 32,
    AUDEN_RDMA5_FADE_DONE_V920_IRQ = 165 + 32,
    AUDEN_RDMA6_FADE_DONE_V920_IRQ = 166 + 32,
    AUDEN_RDMA7_FADE_DONE_V920_IRQ = 167 + 32,
    AUDEN_RDMA8_FADE_DONE_V920_IRQ = 168 + 32,
    AUDEN_RDMA9_FADE_DONE_V920_IRQ = 169 + 32,
    AUDEN_RDMA10_FADE_DONE_V920_IRQ = 170 + 32,
    AUDEN_RDMA11_FADE_DONE_V920_IRQ = 171 + 32,

    AUDEN_RDMA0_ERR_FLAG_V920_IRQ = 172 + 32,
    AUDEN_RDMA1_ERR_FLAG_V920_IRQ = 173 + 32,
    AUDEN_RDMA2_ERR_FLAG_V920_IRQ = 174 + 32,
    AUDEN_RDMA3_ERR_FLAG_V920_IRQ = 175 + 32,
    AUDEN_RDMA4_ERR_FLAG_V920_IRQ = 176 + 32,
    AUDEN_RDMA5_ERR_FLAG_V920_IRQ = 177 + 32,
    AUDEN_RDMA6_ERR_FLAG_V920_IRQ = 178 + 32,
    AUDEN_RDMA7_ERR_FLAG_V920_IRQ = 179 + 32,
    AUDEN_RDMA8_ERR_FLAG_V920_IRQ = 180 + 32,
    AUDEN_RDMA9_ERR_FLAG_V920_IRQ = 181 + 32,
    AUDEN_RDMA10_ERR_FLAG_V920_IRQ = 182 + 32,
    AUDEN_RDMA11_ERR_FLAG_V920_IRQ = 183 + 32,

    AUDEN_WDMA0_BUF_FULL_PRE_V920_IRQ = 184 + 32,
    AUDEN_WDMA1_BUF_FULL_PRE_V920_IRQ = 185 + 32,
    AUDEN_WDMA2_BUF_FULL_PRE_V920_IRQ = 186 + 32,
    AUDEN_WDMA3_BUF_FULL_PRE_V920_IRQ = 187 + 32,
    AUDEN_WDMA4_BUF_FULL_PRE_V920_IRQ = 188 + 32,
    AUDEN_WDMA5_BUF_FULL_PRE_V920_IRQ = 189 + 32,
    AUDEN_WDMA6_BUF_FULL_PRE_V920_IRQ = 190 + 32,
    AUDEN_WDMA7_BUF_FULL_PRE_V920_IRQ = 191 + 32,

    AUDEN_WDMA0_BUF_FULL_V920_IRQ  = 192 + 32,
    AUDEN_WDMA1_BUF_FULL_V920_IRQ  = 193 + 32,
    AUDEN_WDMA2_BUF_FULL_V920_IRQ  = 194 + 32,
    AUDEN_WDMA3_BUF_FULL_V920_IRQ  = 195 + 32,
    AUDEN_WDMA4_BUF_FULL_V920_IRQ  = 196 + 32,
    AUDEN_WDMA5_BUF_FULL_V920_IRQ  = 197 + 32,
    AUDEN_WDMA6_BUF_FULL_V920_IRQ  = 198 + 32,
    AUDEN_WDMA7_BUF_FULL_V920_IRQ  = 199 + 32,

    AUDEN_WDMA0_FADE_DONE_V920_IRQ = 200 + 32,
    AUDEN_WDMA1_FADE_DONE_V920_IRQ = 201 + 32,
    AUDEN_WDMA2_FADE_DONE_V920_IRQ = 202 + 32,
    AUDEN_WDMA3_FADE_DONE_V920_IRQ = 203 + 32,
    AUDEN_WDMA4_FADE_DONE_V920_IRQ = 204 + 32,
    AUDEN_WDMA5_FADE_DONE_V920_IRQ = 205 + 32,
    AUDEN_WDMA6_FADE_DONE_V920_IRQ = 206 + 32,
    AUDEN_WDMA7_FADE_DONE_V920_IRQ = 207 + 32,

    AUDEN_WDMA0_ERR_FLAG_V920_IRQ  = 208 + 32,
    AUDEN_WDMA1_ERR_FLAG_V920_IRQ  = 209 + 32,
    AUDEN_WDMA2_ERR_FLAG_V920_IRQ  = 210 + 32,
    AUDEN_WDMA3_ERR_FLAG_V920_IRQ  = 211 + 32,
    AUDEN_WDMA4_ERR_FLAG_V920_IRQ  = 212 + 32,
    AUDEN_WDMA5_ERR_FLAG_V920_IRQ  = 213 + 32,
    AUDEN_WDMA6_ERR_FLAG_V920_IRQ  = 214 + 32,
    AUDEN_WDMA7_ERR_FLAG_V920_IRQ  = 215 + 32,

    UAIF0_SPEAKER0_V920_IRQ        = 216 + 32,
    UAIF0_MIC0_V920_IRQ            = 217 + 32,
    UAIF0_SPEAKER1_V920_IRQ        = 218 + 32,
    UAIF0_MIC1_V920_IRQ            = 219 + 32,
    UAIF0_SPEAKER2_V920_IRQ        = 220 + 32,
    UAIF0_MIC2_V920_IRQ            = 221 + 32,
    UAIF0_SPEAKER3_V920_IRQ        = 222 + 32,
    UAIF0_MIC3_V920_IRQ            = 223 + 32,

    UAIF1_SPEAKER0_V920_IRQ        = 224 + 32,
    UAIF1_MIC0_V920_IRQ            = 225 + 32,
    UAIF1_SPEAKER1_V920_IRQ        = 226 + 32,
    UAIF1_MIC1_V920_IRQ            = 227 + 32,
    UAIF1_SPEAKER2_V920_IRQ        = 228 + 32,
    UAIF1_MIC2_V920_IRQ            = 229 + 32,
    UAIF1_SPEAKER3_V920_IRQ        = 230 + 32,
    UAIF1_MIC3_V920_IRQ            = 231 + 32,

    UAIF2_SPEAKER0_V920_IRQ        = 232 + 32,
    UAIF2_MIC0_V920_IRQ            = 233 + 32,
    UAIF2_SPEAKER1_V920_IRQ        = 234 + 32,
    UAIF2_MIC1_V920_IRQ            = 235 + 32,

    UAIF3_SPEAKER0_V920_IRQ        = 236 + 32,
    UAIF3_MIC0_V920_IRQ            = 237 + 32,
    UAIF3_SPEAKER1_V920_IRQ        = 238 + 32,
    UAIF3_MIC1_V920_IRQ            = 239 + 32,

    UAIF4_SPEAKER0_V920_IRQ        = 240 + 32,
    UAIF4_MIC0_V920_IRQ            = 241 + 32,
    UAIF4_SPEAKER1_V920_IRQ        = 242 + 32,
    UAIF4_MIC1_V920_IRQ            = 243 + 32,

    UAIF5_SPEAKER0_V920_IRQ        = 244 + 32,
    UAIF5_MIC0_V920_IRQ            = 245 + 32,
    UAIF5_SPEAKER1_V920_IRQ        = 246 + 32,
    UAIF5_MIC1_V920_IRQ            = 247 + 32,

    UAIF6_SPEAKER0_V920_IRQ        = 248 + 32,
    UAIF6_MIC0_V920_IRQ            = 249 + 32,
    UAIF6_SPEAKER1_V920_IRQ        = 250 + 32,
    UAIF6_MIC1_V920_IRQ            = 251 + 32,

    UAIF7_SPEAKER0_V920_IRQ        = 252 + 32,
    UAIF7_MIC0_V920_IRQ            = 253 + 32,
    UAIF7_SPEAKER1_V920_IRQ        = 254 + 32,
    UAIF7_MIC1_V920_IRQ            = 255 + 32,

    UAIF8_SPEAKER0_V920_IRQ        = 256 + 32,
    UAIF8_MIC0_V920_IRQ            = 257 + 32,
    UAIF8_SPEAKER1_V920_IRQ        = 258 + 32,
    UAIF8_MIC1_V920_IRQ            = 259 + 32,

    UAIF9_SPEAKER0_V920_IRQ        = 260 + 32,
    UAIF9_MIC0_V920_IRQ            = 261 + 32,
    UAIF9_SPEAKER1_V920_IRQ        = 262 + 32,
    UAIF9_MIC1_V920_IRQ            = 263 + 32,

    UAIFS_SPEAKER0_V920_IRQ        = 264 + 32,
    UAIFS_MIC0_V920_IRQ            = 265 + 32,
    UAIFS_SPEAKER1_V920_IRQ        = 266 + 32,
    UAIFS_MIC1_V920_IRQ            = 267 + 32,

    WDT0_INTR_IN_V920_IRQ          = 268 + 32,
    WDT1_INTR_IN_V920_IRQ          = 269 + 32,
    WDT2_INTR_IN_V920_IRQ          = 270 + 32,
    WDT3_INTR_IN_V920_IRQ          = 271 + 32,

    HIFI5_IRQ0_V920_IRQ            = 272 + 32,
    HIFI5_IRQ1_V920_IRQ            = 273 + 32,
    HIFI5_IRQ2_V920_IRQ            = 274 + 32,

    RESERVED_275_V920_IRQ          = 275 + 32,
    RESERVED_276_V920_IRQ          = 276 + 32,
    RESERVED_277_V920_IRQ          = 277 + 32,
    RESERVED_278_V920_IRQ          = 278 + 32,

    GPIO_INTR_IN0_V920_IRQ         = 279 + 32,
    GPIO_INTR_IN1_V920_IRQ         = 280 + 32,
    MBOX_SFI_INTR_IN0_V920_IRQ     = 281 + 32,

    RESERVED_282_V920_IRQ          = 282 + 32,

    INTR_UAIF0_HOLD0_V920_IRQ      = 283 + 32,
    INTR_UAIF0_HOLD1_V920_IRQ      = 284 + 32,
    INTR_UAIF0_HOLD2_V920_IRQ      = 285 + 32,
    INTR_UAIF0_HOLD3_V920_IRQ      = 286 + 32,
    INTR_UAIF0_RESUME0_V920_IRQ    = 287 + 32,
    INTR_UAIF0_RESUME2_V920_IRQ    = 288 + 32,
    INTR_UAIF0_RESUME3_V920_IRQ    = 289 + 32,
    INTR_UAIF0_RESUME4_V920_IRQ    = 290 + 32,

    INTR_UAIF1_HOLD0_V920_IRQ      = 291 + 32,
    INTR_UAIF1_HOLD1_V920_IRQ      = 292 + 32,
    INTR_UAIF1_HOLD2_V920_IRQ      = 293 + 32,
    INTR_UAIF1_HOLD3_V920_IRQ      = 294 + 32,
    INTR_UAIF1_RESUME0_V920_IRQ    = 295 + 32,
    INTR_UAIF1_RESUME1_V920_IRQ    = 296 + 32,
    INTR_UAIF1_RESUME2_V920_IRQ    = 297 + 32,
    INTR_UAIF1_RESUME3_V920_IRQ    = 298 + 32,

    INTR_UAIF2_HOLD0_V920_IRQ      = 299 + 32,
    INTR_UAIF2_HOLD1_V920_IRQ      = 300 + 32,
    INTR_UAIF2_RESUME0_V920_IRQ    = 301 + 32,
    INTR_UAIF2_RESUME1_V920_IRQ    = 302 + 32,

    INTR_UAIF3_HOLD0_V920_IRQ      = 303 + 32,
    INTR_UAIF3_HOLD1_V920_IRQ      = 304 + 32,
    INTR_UAIF3_RESUME0_V920_IRQ    = 305 + 32,
    INTR_UAIF3_RESUME1_V920_IRQ    = 306 + 32,

    INTR_UAIF4_HOLD0_V920_IRQ      = 307 + 32,
    INTR_UAIF4_HOLD1_V920_IRQ      = 308 + 32,
    INTR_UAIF4_RESUME0_V920_IRQ    = 309 + 32,
    INTR_UAIF4_RESUME1_V920_IRQ    = 310 + 32,

    INTR_UAIF5_HOLD0_V920_IRQ      = 311 + 32,
    INTR_UAIF5_HOLD1_V920_IRQ      = 312 + 32,
    INTR_UAIF5_RESUME0_V920_IRQ    = 313 + 32,
    INTR_UAIF5_RESUME1_V920_IRQ    = 314 + 32,

    INTR_UAIF6_HOLD0_V920_IRQ      = 315 + 32,
    INTR_UAIF6_HOLD1_V920_IRQ      = 316 + 32,
    INTR_UAIF6_RESUME0_V920_IRQ    = 317 + 32,
    INTR_UAIF6_RESUME1_V920_IRQ    = 318 + 32,

    INTR_UAIF7_HOLD0_V920_IRQ      = 319 + 32,
    INTR_UAIF7_HOLD1_V920_IRQ      = 320 + 32,
    INTR_UAIF7_RESUME0_V920_IRQ    = 321 + 32,
    INTR_UAIF7_RESUME1_V920_IRQ    = 322 + 32,

    INTR_UAIF8_HOLD0_V920_IRQ      = 323 + 32,
    INTR_UAIF8_HOLD1_V920_IRQ      = 324 + 32,
    INTR_UAIF8_RESUME0_V920_IRQ    = 325 + 32,
    INTR_UAIF8_RESUME1_V920_IRQ    = 326 + 32,

    INTR_UAIF9_HOLD0_V920_IRQ      = 327 + 32,
    INTR_UAIF9_HOLD1_V920_IRQ      = 328 + 32,
    INTR_UAIF9_RESUME0_V920_IRQ    = 329 + 32,
    INTR_UAIF9_RESUME1_V920_IRQ    = 330 + 32,

    INTR_UAIFS_HOLD0_V920_IRQ      = 331 + 32,
    INTR_UAIFS_HOLD1_V920_IRQ      = 332 + 32,
    INTR_UAIFS_RESUME0_V920_IRQ    = 333 + 32,
    INTR_UAIFS_RESUME1_V920_IRQ    = 334 + 32,

    INTR_UAIFS_LOOPBACK_OUT_COMP_ERR_V920_IRQ = 335 + 32,
    RESERVED_336_V920_IRQ          = 336 + 32,
    INTR_UAIFS_I2S_COMP_ERR_V920_IRQ = 337 + 32,
    RESERVED_338_V920_IRQ          = 338 + 32,

    TIMER0_V920_IRQ                = 339 + 32,
    TIMER1_V920_IRQ                = 340 + 32,
    TIMER2_V920_IRQ                = 341 + 32,
    TIMER3_V920_IRQ                = 342 + 32,
    TIMER4_V920_IRQ                = 343 + 32,
    TIMER5_V920_IRQ                = 344 + 32,
    TIMER6_V920_IRQ                = 345 + 32,
    TIMER7_V920_IRQ                = 346 + 32,
    TIMER8_V920_IRQ                = 347 + 32,
    TIMER9_V920_IRQ                = 348 + 32,
    TIMER10_V920_IRQ               = 349 + 32,
    TIMER11_V920_IRQ               = 350 + 32,
    TIMER12_V920_IRQ               = 351 + 32,
    TIMER13_V920_IRQ               = 352 + 32,
    TIMER14_V920_IRQ               = 353 + 32,
    TIMER15_V920_IRQ               = 354 + 32,

    MBOX0_INTR0_V920_IRQ           = 355 + 32,
    MBOX0_INTR1_V920_IRQ           = 356 + 32,
    MBOX0_INTR2_V920_IRQ           = 357 + 32,
    MBOX0_INTR3_V920_IRQ           = 358 + 32,

    MBOX1_INTR0_V920_IRQ           = 359 + 32,
    MBOX1_INTR1_V920_IRQ           = 360 + 32,
    MBOX1_INTR2_V920_IRQ           = 361 + 32,
    MBOX1_INTR3_V920_IRQ           = 362 + 32,

    MBOX2_INTR0_V920_IRQ           = 363 + 32,
    MBOX2_INTR1_V920_IRQ           = 364 + 32,
    MBOX2_INTR2_V920_IRQ           = 365 + 32,
    MBOX2_INTR3_V920_IRQ           = 366 + 32,

    MBOX3_INTR0_V920_IRQ           = 367 + 32,
    MBOX3_INTR1_V920_IRQ           = 368 + 32,
    MBOX3_INTR2_V920_IRQ           = 369 + 32,
    MBOX3_INTR3_V920_IRQ           = 370 + 32,

    MBOX4_INTR0_V920_IRQ           = 371 + 32,
    MBOX4_INTR1_V920_IRQ           = 372 + 32,
    MBOX4_INTR2_V920_IRQ           = 373 + 32,
    MBOX4_INTR3_V920_IRQ           = 374 + 32,

    MBOX5_INTR0_V920_IRQ           = 375 + 32,
    MBOX5_INTR1_V920_IRQ           = 376 + 32,
    MBOX5_INTR2_V920_IRQ           = 377 + 32,
    MBOX5_INTR3_V920_IRQ           = 378 + 32,

    MBOX6_INTR0_V920_IRQ           = 379 + 32,
    MBOX6_INTR1_V920_IRQ           = 380 + 32,
    MBOX6_INTR2_V920_IRQ           = 381 + 32,
    MBOX6_INTR3_V920_IRQ           = 382 + 32,

    MBOX7_INTR0_V920_IRQ           = 383 + 32,
    MBOX7_INTR1_V920_IRQ           = 384 + 32,
    MBOX7_INTR2_V920_IRQ           = 385 + 32,
    MBOX7_INTR3_V920_IRQ           = 386 + 32,

    GLUE_UART_PERIAUD_V920_IRQ     = 387 + 32,
    GLUE_UART_I2C2_V920_IRQ        = 388 + 32,
    GLUE_UART_I2C1_V920_IRQ        = 389 + 32,
    GLUE_UART_I2C0_V920_IRQ        = 390 + 32,

    INTREQ__ETH1_PHY_INT            = 391 + 32,
    INTREQ__ETH0_PHY_INT            = 392 + 32,
    INTREQ__ETH1_PTP_PPS3_INT       = 393 + 32,
    INTREQ__ETH1_PTP_PPS2_INT       = 394 + 32,
    INTREQ__ETH0_PTP_PPS2_INT       = 395 + 32,

    INTREQ__ETH1_MCGR_REQ3_INT      = 396 + 32,
    INTREQ__ETH1_MCGR_REQ2_INT      = 397 + 32,
    INTREQ__ETH1_MCGR_REQ1_INT      = 398 + 32,
    INTREQ__ETH1_MCGR_REQ0_INT      = 399 + 32,
    INTREQ__ETH1_SS_WDT_INT         = 400 + 32,
    INTREQ__ETH1_SS_APB_PAR_INT     = 401 + 32,
    INTREQ__ETH1_PCS_GLOBAL_INT     = 402 + 32,
    INTREQ__ETH1_RXDMA15_V920_IRQ  = 403 + 32,
    INTREQ__ETH1_RXDMA14_V920_IRQ  = 404 + 32,
    INTREQ__ETH1_RXDMA13_V920_IRQ  = 405 + 32,
    INTREQ__ETH1_RXDMA12_V920_IRQ  = 406 + 32,
    INTREQ__ETH1_RXDMA11_V920_IRQ  = 407 + 32,
    INTREQ__ETH1_RXDMA10_V920_IRQ  = 408 + 32,
    INTREQ__ETH1_RXDMA09_V920_IRQ  = 409 + 32,
    INTREQ__ETH1_RXDMA08_V920_IRQ  = 410 + 32,
    INTREQ__ETH1_RXDMA07_V920_IRQ  = 411 + 32,
    INTREQ__ETH1_RXDMA06_V920_IRQ  = 412 + 32,
    INTREQ__ETH1_RXDMA05_V920_IRQ  = 413 + 32,
    INTREQ__ETH1_RXDMA04_V920_IRQ  = 414 + 32,
    INTREQ__ETH1_RXDMA03_V920_IRQ  = 415 + 32,
    INTREQ__ETH1_RXDMA02_V920_IRQ  = 416 + 32,
    INTREQ__ETH1_RXDMA01_V920_IRQ  = 417 + 32,
    INTREQ__ETH1_RXDMA00_V920_IRQ  = 418 + 32,
    INTREQ__ETH1_TXDMA15_V920_IRQ  = 419 + 32,
    INTREQ__ETH1_TXDMA14_V920_IRQ  = 420 + 32,
    INTREQ__ETH1_TXDMA13_V920_IRQ  = 421 + 32,
    INTREQ__ETH1_TXDMA12_V920_IRQ  = 422 + 32,
    INTREQ__ETH1_TXDMA11_V920_IRQ  = 423 + 32,
    INTREQ__ETH1_TXDMA10_V920_IRQ  = 424 + 32,
    INTREQ__ETH1_TXDMA09_V920_IRQ  = 425 + 32,
    INTREQ__ETH1_TXDMA08_V920_IRQ  = 426 + 32,
    INTREQ__ETH1_TXDMA07_V920_IRQ  = 427 + 32,
    INTREQ__ETH1_TXDMA06_V920_IRQ  = 428 + 32,
    INTREQ__ETH1_TXDMA05_V920_IRQ  = 429 + 32,
    INTREQ__ETH1_TXDMA04_V920_IRQ  = 430 + 32,
    INTREQ__ETH1_TXDMA03_V920_IRQ  = 431 + 32,
    INTREQ__ETH1_TXDMA02_V920_IRQ  = 432 + 32,
    INTREQ__ETH1_TXDMA01_V920_IRQ  = 433 + 32,
    INTREQ__ETH1_TXDMA00_V920_IRQ  = 434 + 32,

    INTREQ__ETH1_PMT_INT            = 435 + 32,
    INTREQ__ETH1_LPI_INT            = 436 + 32,
    INTREQ__ETH1_GLOBAL_INT         = 437 + 32,

    INTREQ__ETH0_MCGR_REQ3_V920_IRQ = 438 + 32,
    INTREQ__ETH0_MCGR_REQ2_V920_IRQ = 439 + 32,
    INTREQ__ETH0_MCGR_REQ1_V920_IRQ = 440 + 32,
    INTREQ__ETH0_MCGR_REQ0_V920_IRQ = 441 + 32,
    INTREQ__ETH0_SS_WDT_V920_IRQ       = 442 + 32,
    INTREQ__ETH0_SS_APB_PAR_V920_IRQ   = 443 + 32,
    INTREQ__ETH0_PCS_GLOBAL_V920_IRQ   = 444 + 32,
    INTREQ__ETH0_RXDMA15_V920_IRQ  = 445 + 32,
    INTREQ__ETH0_RXDMA14_V920_IRQ  = 446 + 32,
    INTREQ__ETH0_RXDMA13_V920_IRQ  = 447 + 32,
    INTREQ__ETH0_RXDMA12_V920_IRQ  = 448 + 32,
    INTREQ__ETH0_RXDMA11_V920_IRQ  = 449 + 32,
    INTREQ__ETH0_RXDMA10_V920_IRQ  = 450 + 32,
    INTREQ__ETH0_RXDMA09_V920_IRQ  = 451 + 32,
    INTREQ__ETH0_RXDMA08_V920_IRQ  = 452 + 32,
    INTREQ__ETH0_RXDMA07_V920_IRQ  = 453 + 32,
    INTREQ__ETH0_RXDMA06_V920_IRQ  = 454 + 32,
    INTREQ__ETH0_RXDMA05_V920_IRQ  = 455 + 32,
    INTREQ__ETH0_RXDMA04_V920_IRQ  = 456 + 32,
    INTREQ__ETH0_RXDMA03_V920_IRQ  = 457 + 32,
    INTREQ__ETH0_RXDMA02_V920_IRQ  = 458 + 32,
    INTREQ__ETH0_RXDMA01_V920_IRQ  = 459 + 32,
    INTREQ__ETH0_RXDMA00_V920_IRQ  = 460 + 32,
    INTREQ__ETH0_TXDMA15_V920_IRQ  = 461 + 32,
    INTREQ__ETH0_TXDMA14_V920_IRQ  = 462 + 32,
    INTREQ__ETH0_TXDMA13_V920_IRQ  = 463 + 32,
    INTREQ__ETH0_TXDMA12_V920_IRQ  = 464 + 32,
    INTREQ__ETH0_TXDMA11_V920_IRQ  = 465 + 32,
    INTREQ__ETH0_TXDMA10_V920_IRQ  = 466 + 32,
    INTREQ__ETH0_TXDMA09_V920_IRQ  = 467 + 32,
    INTREQ__ETH0_TXDMA08_V920_IRQ  = 468 + 32,
    INTREQ__ETH0_TXDMA07_V920_IRQ  = 469 + 32,
    INTREQ__ETH0_TXDMA06_V920_IRQ  = 470 + 32,
    INTREQ__ETH0_TXDMA05_V920_IRQ  = 471 + 32,
    INTREQ__ETH0_TXDMA04_V920_IRQ  = 472 + 32,
    INTREQ__ETH0_TXDMA03_V920_IRQ  = 473 + 32,
    INTREQ__ETH0_TXDMA02_V920_IRQ  = 474 + 32,
    INTREQ__ETH0_TXDMA01_V920_IRQ  = 475 + 32,
    INTREQ__ETH0_TXDMA00_V920_IRQ  = 476 + 32,

    INTREQ__ETH0_PMT_V920_IRQ      = 477 + 32,
    INTREQ__ETH0_LPI_V920_IRQ      = 478 + 32,
    INTREQ__ETH0_GLOBAL_V920_IRQ   = 479 + 32,

    ABOX_GIC_IRQ_MAXNUM             = 480 + 32, //IRQn_MAXNUM
    } ABOX_V920_GIC_IRQ;

typedef STATUS (*ABOX_GIC_IRQ_HANDLE_FUNC)(uint32_t, VXB_DEV_ID);

typedef struct aboxGicIrqHandler
    {
    ABOX_GIC_IRQ_HANDLE_FUNC    irqFunc;
    VXB_DEV_ID                  devId;
    } ABOX_GIC_IRQ_HANDLER_T;

typedef struct aboxGicData
    {
    VXB_DEV_ID                  pDev;
    ABOX_CTR_DATA *             pCtrData;
    VIRT_ADDR                   gicdRegBase;
    VIRT_ADDR                   giccRegBase;
    VIRT_ADDR                   iarBakRegBase;
    void *                      regHandle;
    uint32_t                    gicdSettingMode;
    VXB_RESOURCE *              intRes;

    ABOX_GIC_IRQ_HANDLER_T      gicIrqHandler[ABOX_GIC_IRQ_MAXNUM];
    ABOX_GIC_IRQ_HANDLER_T *    pcmIrqHandler;
    uint32_t                    pcmIrqNum;
    uint32_t                    cpuId;
    uint32_t                    domain;

    TASK_ID                     irqTask;
    spinlockIsr_t               irqQueSpinlockIsr;
    SEM_ID                      irqHandleSem;
    BOOL                        irqWaitFlag;
    uint32_t                    irqQue[ABOX_GIC_IRQ_QUEUE_SIZE];
    uint32_t                    irqQueStart;
    uint32_t                    irqQueEnd;
    }ABOX_GIC_DATA;

extern STATUS aboxGicRegisterSpiHandler
    (
    VXB_DEV_ID                  gicDev,
    uint32_t                    spiId,
    ABOX_GIC_IRQ_HANDLE_FUNC    irqFunc,
    VXB_DEV_ID                  devId
    );

extern STATUS abox_gic_set_pcm_irq_register
    (
    uint32_t                    pcmIrqId,
    ABOX_GIC_IRQ_HANDLE_FUNC    irqFunc,
    VXB_DEV_ID                  devId
    );

extern STATUS aboxGicInit
    (
    ABOX_CTR_DATA * pCtrData,
    uint32_t        domain
    );

extern void aboxGicForwardSpi
    (
    uint32_t  spiId
    );

extern STATUS abox_gic_set_pcm_irq_register
    (
    uint32_t                    pcmIrqId,
    ABOX_GIC_IRQ_HANDLE_FUNC    irqFunc,
    VXB_DEV_ID                  devId
    );



#ifdef __cplusplus
    }
#endif

#endif /* __INCvxbFdtAboxGich */

