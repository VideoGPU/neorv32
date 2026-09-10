// ================================================================================ //
// The NEORV32 RISC-V Processor - https://github.com/stnolting/neorv32              //
// Copyright (c) NEORV32 contributors.                                              //
// Copyright (c) 2020 - 2026 Stephan Nolting. All rights reserved.                  //
// Licensed under the BSD-3-Clause license, see LICENSE for details.                //
// SPDX-License-Identifier: BSD-3-Clause                                            //
// ================================================================================ //

/**
 * @file neorv32_mipi_tx.h
 * @brief MIPI CSI-2 TX AXI4-Lite IP HW driver header file.
 *
 * @note This is not a NEORV32 SoC module: the IP is an external AXI4-Lite slave
 * attached to the processor's external bus interface (XBUS).
 */

#ifndef NEORV32_MIPI_TX_H
#define NEORV32_MIPI_TX_H

#include <neorv32.h>
#include <stdint.h>

/**********************************************************************//**
 * @name External IP: MIPI CSI-2 TX (MIPI_TX)
 **************************************************************************/
/**@{*/
/** MIPI_TX module prototype */
typedef volatile struct __attribute__((packed,aligned(4))) {
  uint32_t CONTROL;            /**< offset 0x00: control register (#NEORV32_MIPI_TX_CTRL_enum) */
  uint32_t STATUS;             /**< offset 0x04: status register (#NEORV32_MIPI_TX_STATUS_enum) */
  const uint32_t reserved[2];  /**< offset 0x08..0x0c: reserved */
  uint32_t PIXELS_PER_LINE;    /**< offset 0x10: payload bytes per line, 16-bit */
  uint32_t N_LINES;            /**< offset 0x14: lines per frame, 16-bit */
  uint32_t TYPE_VC;            /**< offset 0x18: data type / virtual channel (#NEORV32_MIPI_TX_TYPE_VC_enum) */
  uint32_t FRAME_END_WORD;     /**< offset 0x1c: Frame End short packet payload, 16-bit */
  uint32_t TLP_SOT_DELAY_CLK;  /**< offset 0x20: clock lane tLP-SOT delay, 16-bit */
  uint32_t TLPX_DELAY_CLK;     /**< offset 0x24: clock lane tLPX delay, 16-bit */
  uint32_t TLP_SOT_DELAY_DATA; /**< offset 0x28: data lane tLP-SOT delay, 16-bit */
  uint32_t TLPX_DELAY_DATA;    /**< offset 0x2c: data lane tLPX delay, 16-bit */
  uint32_t TLP_SOT_SHORT;      /**< offset 0x30: extra tLP-SOT delay for short packets, 16-bit */
  uint32_t THS_PREPARE;        /**< offset 0x34: tHS-PREPARE, 16-bit */
  uint32_t THS_ZERO;           /**< offset 0x38: tHS-ZERO, 16-bit */
  uint32_t THS_EXIT;           /**< offset 0x3c: tHS-EXIT, 16-bit */
  uint32_t N_MIPI_LANES;       /**< offset 0x40: lane count encoding (#NEORV32_MIPI_TX_LANES_enum) */
} neorv32_mipi_tx_t;

/** MIPI_TX module hardware handle (#neorv32_mipi_tx_t) */
#define NEORV32_MIPI_TX ((neorv32_mipi_tx_t*) (NEORV32_MIPI_TX_BASE))

/** MIPI_TX control register bits */
enum NEORV32_MIPI_TX_CTRL_enum {
  MIPI_TX_CTRL_START       = 0, /**< MIPI_TX control register(0) (-/w): start frame, latches configuration, reads as zero */
  MIPI_TX_CTRL_STOP        = 1, /**< MIPI_TX control register(1) (r/w): abort frame after the current line */
  MIPI_TX_CTRL_IRQ_EN      = 2, /**< MIPI_TX control register(2) (r/w): gate the IP's interrupt output */
  MIPI_TX_CTRL_LEGACY_TRIG = 3, /**< MIPI_TX control register(3) (r/w): legacy hardware trigger enable */
  MIPI_TX_CTRL_IRQ_CLR     = 4, /**< MIPI_TX control register(4) (-/w): clear pending interrupt, reads as zero */
  MIPI_TX_CTRL_OVERLAY_EN  = 5  /**< MIPI_TX control register(5) (r/w): enable debug overlay pattern */
};

/** MIPI_TX status register bits */
enum NEORV32_MIPI_TX_STATUS_enum {
  MIPI_TX_STATUS_HS_ACTIVE  = 0, /**< MIPI_TX status register(0) (r/-): data lanes in high-speed mode */
  MIPI_TX_STATUS_HS_VALID   = 1, /**< MIPI_TX status register(1) (r/-): high-speed payload valid */
  MIPI_TX_STATUS_FRAME_DONE = 2, /**< MIPI_TX status register(2) (r/-): frame transmission completed */
  MIPI_TX_STATUS_IRQ_PEND   = 3  /**< MIPI_TX status register(3) (r/-): interrupt pending, clear via #MIPI_TX_CTRL_IRQ_CLR */
};

/** MIPI_TX data type / virtual channel register bits */
enum NEORV32_MIPI_TX_TYPE_VC_enum {
  MIPI_TX_TYPE_VC_DT_LSB = 0, /**< MIPI_TX type/vc register(0) (r/w): CSI-2 data type, LSB */
  MIPI_TX_TYPE_VC_DT_MSB = 5, /**< MIPI_TX type/vc register(5) (r/w): CSI-2 data type, MSB */
  MIPI_TX_TYPE_VC_VC_LSB = 8, /**< MIPI_TX type/vc register(8) (r/w): virtual channel, LSB */
  MIPI_TX_TYPE_VC_VC_MSB = 9  /**< MIPI_TX type/vc register(9) (r/w): virtual channel, MSB */
};

/** MIPI_TX lane count register encoding */
enum NEORV32_MIPI_TX_LANES_enum {
  MIPI_TX_LANES_1 = 1, /**< 1 data lane */
  MIPI_TX_LANES_2 = 2, /**< 2 data lanes */
  MIPI_TX_LANES_4 = 3  /**< 4 data lanes */
};

/** MIPI_TX CSI-2 data type codes */
enum NEORV32_MIPI_TX_DT_enum {
  MIPI_TX_DT_FRAME_START = 0x00, /**< short packet: frame start */
  MIPI_TX_DT_FRAME_END   = 0x01, /**< short packet: frame end */
  MIPI_TX_DT_LINE_START  = 0x02, /**< short packet: line start */
  MIPI_TX_DT_LINE_END    = 0x03, /**< short packet: line end */
  MIPI_TX_DT_RAW8        = 0x2a, /**< long packet: RAW8 */
  MIPI_TX_DT_RAW10       = 0x2b, /**< long packet: RAW10 */
  MIPI_TX_DT_RAW12       = 0x2c  /**< long packet: RAW12 */
};

/** MIPI_TX D-PHY timing configuration, all values in 10ns ticks of clk_DPHY_100Mhz */
typedef struct {
  uint16_t tlp_sot_delay_clk;  /**< clock lane LP-to-HS entry delay */
  uint16_t tlpx_delay_clk;     /**< clock lane tLPX */
  uint16_t tlp_sot_delay_data; /**< data lane LP-to-HS entry delay */
  uint16_t tlpx_delay_data;    /**< data lane tLPX */
  uint16_t tlp_sot_short;      /**< extra LP-to-HS delay for short packets */
  uint16_t ths_prepare;        /**< tHS-PREPARE */
  uint16_t ths_zero;           /**< tHS-ZERO */
  uint16_t ths_exit;           /**< tHS-EXIT */
} neorv32_mipi_tx_timing_t;

/** MIPI_TX frame configuration */
typedef struct {
  uint8_t  n_mipi_lanes;    /**< number of data lanes: 1, 2 or 4 */
  uint16_t pixels_per_line; /**< payload bytes per line */
  uint16_t n_lines;         /**< lines per frame */
  uint8_t  data_type;       /**< CSI-2 data type (#NEORV32_MIPI_TX_DT_enum), 6-bit */
  uint8_t  vc;              /**< virtual channel, 2-bit */
  uint16_t frame_end_word;  /**< Frame End short packet payload */
  neorv32_mipi_tx_timing_t timing; /**< D-PHY timing */
  int irq_enable;           /**< enable the IP's interrupt output */
  int legacy_trigger_en;    /**< enable the legacy hardware trigger */
  int debug_overlay_en;     /**< enable the debug overlay pattern */
} neorv32_mipi_tx_config_t;
/**@}*/


/**********************************************************************//**
 * @name Prototypes
 **************************************************************************/
/**@{*/
int  neorv32_mipi_tx_available(void);
void neorv32_mipi_tx_get_default_config(neorv32_mipi_tx_config_t *cfg);
void neorv32_mipi_tx_apply_config(const neorv32_mipi_tx_config_t *cfg);
void neorv32_mipi_tx_set_geometry(uint16_t pixels_per_line, uint16_t n_lines);
void neorv32_mipi_tx_set_lanes(int n_mipi_lanes);
void neorv32_mipi_tx_set_data_type(int data_type, int vc);
void neorv32_mipi_tx_set_frame_end_word(uint16_t frame_end_word);
void neorv32_mipi_tx_set_dphy_timing(const neorv32_mipi_tx_timing_t *timing);
void neorv32_mipi_tx_set_debug_overlay(int enable);
void neorv32_mipi_tx_start(void);
void neorv32_mipi_tx_request_stop(int enable);
uint32_t neorv32_mipi_tx_get_status(void);
int  neorv32_mipi_tx_hs_active(void);
int  neorv32_mipi_tx_frame_done(void);
int  neorv32_mipi_tx_irq_pending(void);
void neorv32_mipi_tx_irq_config(int enable);
void neorv32_mipi_tx_clear_irq(void);
int  neorv32_mipi_tx_wait_frame_done(uint32_t timeout);
/**@}*/


/**********************************************************************//**
 * Convert a RAW10 pixel count into the payload byte count expected by
 * #neorv32_mipi_tx_set_geometry.
 *
 * @param[in] pixels Number of RAW10 pixels per line (should be a multiple of 4).
 * @return Payload size in bytes.
 **************************************************************************/
static inline uint16_t __attribute__ ((always_inline)) neorv32_mipi_tx_raw10_bytes(uint16_t pixels) {

  return (uint16_t)(((uint32_t)pixels * 10) / 8);
}


#endif // NEORV32_MIPI_TX_H
