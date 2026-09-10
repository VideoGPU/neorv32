// ================================================================================ //
// The NEORV32 RISC-V Processor - https://github.com/stnolting/neorv32              //
// Copyright (c) NEORV32 contributors.                                              //
// Copyright (c) 2020 - 2026 Stephan Nolting. All rights reserved.                  //
// Licensed under the BSD-3-Clause license, see LICENSE for details.                //
// SPDX-License-Identifier: BSD-3-Clause                                            //
// ================================================================================ //


/**********************************************************************//**
 * @file demo_mipi_tx/main.c
 * @brief MIPI CSI-2 TX AXI IP usage example.
 **************************************************************************/

#include <neorv32.h>


/**********************************************************************//**
 * @name User configuration
 **************************************************************************/
/**@{*/
/** UART BAUD rate */
#define BAUD_RATE 19200
/** Number of frames to transmit */
#define NUM_FRAMES 4
/** Line length in RAW10 pixels */
#define FRAME_PIXELS 640
/** Number of lines per frame */
#define FRAME_LINES 480
/** Frame-done poll timeout */
#define FRAME_TIMEOUT 10000000
/** Use the machine external interrupt instead of polling.
 * Requires the IP's irq output to be wired to the processor's irq_mei_i port. */
#ifndef USE_IRQ
#define USE_IRQ 0
#endif
/**@}*/

volatile int frame_irq_count = 0;


/**********************************************************************//**
 * MIPI TX machine external interrupt handler.
 **************************************************************************/
void mipi_tx_mei_handler(void) {

  neorv32_mipi_tx_clear_irq();
  frame_irq_count++;
}


/**********************************************************************//**
 * Configure the MIPI CSI-2 TX IP and transmit a couple of test frames.
 *
 * @note This program requires UART0 and the MIPI TX IP attached to the XBUS.
 *
 * @return 0 if execution completed.
 **************************************************************************/
int main() {

  // setup NEORV32 runtime environment
  neorv32_rte_setup();

  // setup UART at default baud rate, no interrupts
  neorv32_uart0_setup(BAUD_RATE, 0);

  neorv32_uart0_printf("MIPI CSI-2 TX demo program\n\n");

  if (neorv32_mipi_tx_available() == 0) {
    neorv32_uart0_printf("ERROR! MIPI TX IP not detected at 0x%x!\n", (uint32_t)NEORV32_MIPI_TX_BASE);
    return 1;
  }

  // start from the IP's reset defaults and shrink the frame
  neorv32_mipi_tx_config_t cfg;
  neorv32_mipi_tx_get_default_config(&cfg);
  cfg.n_mipi_lanes    = 2;
  cfg.pixels_per_line = neorv32_mipi_tx_raw10_bytes(FRAME_PIXELS);
  cfg.n_lines         = FRAME_LINES;
  cfg.data_type       = MIPI_TX_DT_RAW10;
  cfg.vc              = 0;
  cfg.irq_enable      = USE_IRQ;

  neorv32_mipi_tx_apply_config(&cfg);

  neorv32_uart0_printf("Frame: %u bytes/line, %u lines, %u lanes, DT 0x%x\n\n",
                       (uint32_t)cfg.pixels_per_line, (uint32_t)cfg.n_lines,
                       (uint32_t)cfg.n_mipi_lanes, (uint32_t)cfg.data_type);

#if (USE_IRQ != 0)
  neorv32_rte_handler_install(TRAP_CODE_MEI, mipi_tx_mei_handler);
  neorv32_cpu_csr_set(CSR_MIE, 1 << CSR_MIE_MEIE);
  neorv32_cpu_csr_set(CSR_MSTATUS, 1 << CSR_MSTATUS_MIE);

  for (int i = 0; i < NUM_FRAMES; i++) {
    neorv32_mipi_tx_clear_irq();
    neorv32_mipi_tx_start();
    while (frame_irq_count <= i) {
      neorv32_cpu_sleep();
    }
    neorv32_uart0_printf("Frame %u done (IRQ), status 0x%x\n", i, neorv32_mipi_tx_get_status());
  }
#else
  for (int i = 0; i < NUM_FRAMES; i++) {
    neorv32_mipi_tx_clear_irq();
    neorv32_mipi_tx_start();
    if (neorv32_mipi_tx_wait_frame_done(FRAME_TIMEOUT)) {
      neorv32_uart0_printf("ERROR! Frame %u timed out, status 0x%x\n", i, neorv32_mipi_tx_get_status());
      return 1;
    }
    neorv32_uart0_printf("Frame %u done, status 0x%x\n", i, neorv32_mipi_tx_get_status());
  }
  neorv32_mipi_tx_clear_irq();
#endif

  neorv32_uart0_printf("\nDone.\n");
  return 0;
}
