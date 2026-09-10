// ================================================================================ //
// The NEORV32 RISC-V Processor - https://github.com/stnolting/neorv32              //
// Copyright (c) NEORV32 contributors.                                              //
// Copyright (c) 2020 - 2026 Stephan Nolting. All rights reserved.                  //
// Licensed under the BSD-3-Clause license, see LICENSE for details.                //
// SPDX-License-Identifier: BSD-3-Clause                                            //
// ================================================================================ //

/**
 * @file neorv32_mipi_tx.c
 * @brief MIPI CSI-2 TX AXI4-Lite IP HW driver source file.
 */

#include <neorv32.h>
#include <stddef.h>

// the register layout has to match the AXI4-Lite address map of mipi_tx_axi_ip.sv
_Static_assert(offsetof(neorv32_mipi_tx_t, CONTROL)         == 0x00, "MIPI_TX CONTROL offset mismatch");
_Static_assert(offsetof(neorv32_mipi_tx_t, STATUS)          == 0x04, "MIPI_TX STATUS offset mismatch");
_Static_assert(offsetof(neorv32_mipi_tx_t, PIXELS_PER_LINE) == 0x10, "MIPI_TX PIXELS_PER_LINE offset mismatch");
_Static_assert(offsetof(neorv32_mipi_tx_t, TYPE_VC)         == 0x18, "MIPI_TX TYPE_VC offset mismatch");
_Static_assert(offsetof(neorv32_mipi_tx_t, THS_EXIT)        == 0x3c, "MIPI_TX THS_EXIT offset mismatch");
_Static_assert(offsetof(neorv32_mipi_tx_t, N_MIPI_LANES)    == 0x40, "MIPI_TX N_MIPI_LANES offset mismatch");


/**********************************************************************//**
 * Translate a lane count into the N_MIPI_LANES register encoding.
 *
 * @param[in] n_mipi_lanes Number of data lanes: 1, 2 or 4.
 * @return Register encoding (#NEORV32_MIPI_TX_LANES_enum), 2 lanes for illegal input.
 **************************************************************************/
static uint32_t neorv32_mipi_tx_lane_code(int n_mipi_lanes) {

  if (n_mipi_lanes == 1) {
    return (uint32_t)MIPI_TX_LANES_1;
  }
  else if (n_mipi_lanes == 4) {
    return (uint32_t)MIPI_TX_LANES_4;
  }
  else {
    return (uint32_t)MIPI_TX_LANES_2;
  }
}


/**********************************************************************//**
 * Assemble the static part of the control register (everything but the
 * self-clearing start and IRQ-acknowledge bits).
 *
 * @param[in] cfg Frame configuration.
 * @return Control register value.
 **************************************************************************/
static uint32_t neorv32_mipi_tx_ctrl_word(const neorv32_mipi_tx_config_t *cfg) {

  uint32_t tmp = 0;
  tmp |= (uint32_t)((cfg->irq_enable        != 0) & 1U) << MIPI_TX_CTRL_IRQ_EN;
  tmp |= (uint32_t)((cfg->legacy_trigger_en != 0) & 1U) << MIPI_TX_CTRL_LEGACY_TRIG;
  tmp |= (uint32_t)((cfg->debug_overlay_en  != 0) & 1U) << MIPI_TX_CTRL_OVERLAY_EN;
  return tmp;
}


/**********************************************************************//**
 * Check if the MIPI CSI-2 TX IP is reachable.
 *
 * The IP is an external AXI4-Lite slave, so it has no SYSINFO feature bit.
 * This checks that the external bus interface is implemented and that the
 * lane register reads back a legal encoding.
 *
 * @note An unmapped XBUS address raises a load access fault instead of
 * returning here, so the processor's bus timeout has to be configured.
 *
 * @return zero if the IP was not detected, non-zero if it is available.
 **************************************************************************/
int neorv32_mipi_tx_available(void) {

  if ((NEORV32_SYSINFO->SOC & (1 << SYSINFO_SOC_XBUS)) == 0) {
    return 0;
  }

  uint32_t lanes = NEORV32_MIPI_TX->N_MIPI_LANES;
  if ((lanes == 0) || (lanes > (uint32_t)MIPI_TX_LANES_4)) {
    return 0;
  }

  return 1;
}


/**********************************************************************//**
 * Get the frame configuration matching the IP's reset defaults (QSXGA RAW10).
 *
 * @param[out] cfg Frame configuration to populate.
 **************************************************************************/
void neorv32_mipi_tx_get_default_config(neorv32_mipi_tx_config_t *cfg) {

  cfg->n_mipi_lanes    = 2;
  cfg->pixels_per_line = 3240;
  cfg->n_lines         = 1944;
  cfg->data_type       = MIPI_TX_DT_RAW10;
  cfg->vc              = 0;
  cfg->frame_end_word  = 0x1753;

  // D-PHY timing in 10ns ticks
  cfg->timing.tlp_sot_delay_clk  = 42;
  cfg->timing.tlpx_delay_clk     = 10;
  cfg->timing.tlp_sot_delay_data = 187;
  cfg->timing.tlpx_delay_data    = 10;
  cfg->timing.tlp_sot_short      = 783;
  cfg->timing.ths_prepare        = 15;
  cfg->timing.ths_zero           = 80;
  cfg->timing.ths_exit           = 10;

  cfg->irq_enable        = 0;
  cfg->legacy_trigger_en = 1;
  cfg->debug_overlay_en  = 1;
}


/**********************************************************************//**
 * Program the complete frame configuration.
 *
 * The IP latches the configuration registers into the transmit core on the
 * start pulse only, so #neorv32_mipi_tx_start has to be called afterwards.
 *
 * @param[in] cfg Frame configuration.
 **************************************************************************/
void neorv32_mipi_tx_apply_config(const neorv32_mipi_tx_config_t *cfg) {

  neorv32_mipi_tx_set_lanes(cfg->n_mipi_lanes);
  neorv32_mipi_tx_set_geometry(cfg->pixels_per_line, cfg->n_lines);
  neorv32_mipi_tx_set_data_type(cfg->data_type, cfg->vc);
  neorv32_mipi_tx_set_frame_end_word(cfg->frame_end_word);
  neorv32_mipi_tx_set_dphy_timing(&cfg->timing);

  NEORV32_MIPI_TX->CONTROL = neorv32_mipi_tx_ctrl_word(cfg);
}


/**********************************************************************//**
 * Configure the frame geometry.
 *
 * @param[in] pixels_per_line Payload size per line in bytes (not pixels);
 * see #neorv32_mipi_tx_raw10_bytes. Limited by the IP's PIXELS_PER_LINE_MAX generic.
 * @param[in] n_lines Number of lines per frame.
 **************************************************************************/
void neorv32_mipi_tx_set_geometry(uint16_t pixels_per_line, uint16_t n_lines) {

  NEORV32_MIPI_TX->PIXELS_PER_LINE = (uint32_t)pixels_per_line;
  NEORV32_MIPI_TX->N_LINES         = (uint32_t)n_lines;
}


/**********************************************************************//**
 * Configure the number of active data lanes.
 *
 * @param[in] n_mipi_lanes Number of data lanes: 1, 2 or 4. Illegal values select 2 lanes.
 **************************************************************************/
void neorv32_mipi_tx_set_lanes(int n_mipi_lanes) {

  NEORV32_MIPI_TX->N_MIPI_LANES = neorv32_mipi_tx_lane_code(n_mipi_lanes);
}


/**********************************************************************//**
 * Configure the CSI-2 data type and virtual channel of the payload packets.
 *
 * @param[in] data_type CSI-2 data type (#NEORV32_MIPI_TX_DT_enum), 6-bit.
 * @param[in] vc Virtual channel, 2-bit.
 **************************************************************************/
void neorv32_mipi_tx_set_data_type(int data_type, int vc) {

  uint32_t tmp = 0;
  tmp |= ((uint32_t)data_type & 0x3fU) << MIPI_TX_TYPE_VC_DT_LSB;
  tmp |= ((uint32_t)vc        & 0x03U) << MIPI_TX_TYPE_VC_VC_LSB;
  NEORV32_MIPI_TX->TYPE_VC = tmp;
}


/**********************************************************************//**
 * Configure the payload of the Frame End short packet.
 *
 * @param[in] frame_end_word 16-bit frame end marker.
 **************************************************************************/
void neorv32_mipi_tx_set_frame_end_word(uint16_t frame_end_word) {

  NEORV32_MIPI_TX->FRAME_END_WORD = (uint32_t)frame_end_word;
}


/**********************************************************************//**
 * Configure the D-PHY LP/HS transition timing.
 *
 * @param[in] timing Timing set, all values in 10ns ticks of the IP's 100MHz D-PHY clock.
 **************************************************************************/
void neorv32_mipi_tx_set_dphy_timing(const neorv32_mipi_tx_timing_t *timing) {

  NEORV32_MIPI_TX->TLP_SOT_DELAY_CLK  = (uint32_t)timing->tlp_sot_delay_clk;
  NEORV32_MIPI_TX->TLPX_DELAY_CLK     = (uint32_t)timing->tlpx_delay_clk;
  NEORV32_MIPI_TX->TLP_SOT_DELAY_DATA = (uint32_t)timing->tlp_sot_delay_data;
  NEORV32_MIPI_TX->TLPX_DELAY_DATA    = (uint32_t)timing->tlpx_delay_data;
  NEORV32_MIPI_TX->TLP_SOT_SHORT      = (uint32_t)timing->tlp_sot_short;
  NEORV32_MIPI_TX->THS_PREPARE        = (uint32_t)timing->ths_prepare;
  NEORV32_MIPI_TX->THS_ZERO           = (uint32_t)timing->ths_zero;
  NEORV32_MIPI_TX->THS_EXIT           = (uint32_t)timing->ths_exit;
}


/**********************************************************************//**
 * Enable/disable the debug overlay pattern.
 *
 * @param[in] enable Enable overlay when non-zero, disable when zero.
 **************************************************************************/
void neorv32_mipi_tx_set_debug_overlay(int enable) {

  if (enable) {
    __MMREG32_BSET(NEORV32_MIPI_TX->CONTROL, 1 << MIPI_TX_CTRL_OVERLAY_EN);
  }
  else {
    __MMREG32_BCLR(NEORV32_MIPI_TX->CONTROL, 1 << MIPI_TX_CTRL_OVERLAY_EN);
  }
}


/**********************************************************************//**
 * Start the transmission of a single frame.
 *
 * This latches the current configuration registers into the transmit core.
 **************************************************************************/
void neorv32_mipi_tx_start(void) {

  uint32_t tmp = NEORV32_MIPI_TX->CONTROL;
  tmp &= ~((uint32_t)(1 << MIPI_TX_CTRL_IRQ_CLR));
  tmp |=   (uint32_t)(1 << MIPI_TX_CTRL_START);
  NEORV32_MIPI_TX->CONTROL = tmp;
}


/**********************************************************************//**
 * Request/release an abort of the running frame.
 *
 * Unlike the other configuration bits this takes effect immediately, without
 * a start pulse. The bit is level sensitive and has to be released again.
 *
 * @param[in] enable Request abort when non-zero, release the request when zero.
 **************************************************************************/
void neorv32_mipi_tx_request_stop(int enable) {

  if (enable) {
    __MMREG32_BSET(NEORV32_MIPI_TX->CONTROL, 1 << MIPI_TX_CTRL_STOP);
  }
  else {
    __MMREG32_BCLR(NEORV32_MIPI_TX->CONTROL, 1 << MIPI_TX_CTRL_STOP);
  }
}


/**********************************************************************//**
 * Get the raw status register.
 *
 * @return Status register value (#NEORV32_MIPI_TX_STATUS_enum).
 **************************************************************************/
uint32_t neorv32_mipi_tx_get_status(void) {

  return NEORV32_MIPI_TX->STATUS;
}


/**********************************************************************//**
 * Check if the data lanes are in high-speed mode.
 *
 * @return zero if the lanes are in low-power mode, non-zero if they are in high-speed mode.
 **************************************************************************/
int neorv32_mipi_tx_hs_active(void) {

  return (int)(NEORV32_MIPI_TX->STATUS & (1 << MIPI_TX_STATUS_HS_ACTIVE));
}


/**********************************************************************//**
 * Check the frame-done flag.
 *
 * @return zero while a frame is being transmitted, non-zero when it has completed.
 **************************************************************************/
int neorv32_mipi_tx_frame_done(void) {

  return (int)(NEORV32_MIPI_TX->STATUS & (1 << MIPI_TX_STATUS_FRAME_DONE));
}


/**********************************************************************//**
 * Check the latched interrupt flag. This is set on every frame-done edge,
 * independent of #MIPI_TX_CTRL_IRQ_EN, so it can be polled.
 *
 * @return zero if no interrupt is pending, non-zero otherwise.
 **************************************************************************/
int neorv32_mipi_tx_irq_pending(void) {

  return (int)(NEORV32_MIPI_TX->STATUS & (1 << MIPI_TX_STATUS_IRQ_PEND));
}


/**********************************************************************//**
 * Enable/disable the IP's interrupt output.
 *
 * @param[in] enable Enable the interrupt output when non-zero, disable when zero.
 **************************************************************************/
void neorv32_mipi_tx_irq_config(int enable) {

  if (enable) {
    __MMREG32_BSET(NEORV32_MIPI_TX->CONTROL, 1 << MIPI_TX_CTRL_IRQ_EN);
  }
  else {
    __MMREG32_BCLR(NEORV32_MIPI_TX->CONTROL, 1 << MIPI_TX_CTRL_IRQ_EN);
  }
}


/**********************************************************************//**
 * Clear the latched interrupt flag.
 **************************************************************************/
void neorv32_mipi_tx_clear_irq(void) {

  uint32_t tmp = NEORV32_MIPI_TX->CONTROL;
  tmp &= ~((uint32_t)(1 << MIPI_TX_CTRL_START));
  tmp |=   (uint32_t)(1 << MIPI_TX_CTRL_IRQ_CLR);
  NEORV32_MIPI_TX->CONTROL = tmp;
}


/**********************************************************************//**
 * Wait until the running frame has completed.
 *
 * @param[in] timeout Maximum number of poll iterations, zero to wait forever.
 * @return zero on success, -1 on timeout.
 **************************************************************************/
int neorv32_mipi_tx_wait_frame_done(uint32_t timeout) {

  uint32_t cnt = timeout;

  while (neorv32_mipi_tx_irq_pending() == 0) {
    if (timeout) {
      if (cnt == 0) {
        return -1;
      }
      cnt--;
    }
  }

  return 0;
}
