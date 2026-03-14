// ================================================================================ //
// The NEORV32 RISC-V Processor - https://github.com/stnolting/neorv32              //
// Copyright (c) NEORV32 contributors.                                              //
// Copyright (c) 2020 - 2025 Stephan Nolting. All rights reserved.                  //
// Licensed under the BSD-3-Clause license, see LICENSE for details.                //
// SPDX-License-Identifier: BSD-3-Clause                                            //
// ================================================================================ //


/**********************************************************************//**
 * @file demo_twd/main.c
 * @brief TWD loopback demo (I2C slave echo).
 **************************************************************************/

#include <neorv32.h>


/**********************************************************************//**
 * @name User configuration
 **************************************************************************/
/**@{*/
/** UART BAUD rate */
#define BAUD_RATE 19200
/** 7-bit I2C slave address */
#define TWD_ADDR  0x32
/**@}*/


/**********************************************************************//**
 * Main function; receives bytes via TWD and echoes them back.
 *
 * @note Requires UART0 and TWD peripherals.
 *
 * @return 0 if execution was successful
 **************************************************************************/
int main() {

  // capture all exceptions and give debug info via UART
  // this is not required, but keeps us safe
  neorv32_rte_setup();

  // check if UART unit is implemented at all
  if (neorv32_uart0_available() == 0) {
    return 1;
  }

  // setup UART at default baud rate, no interrupts
  neorv32_uart0_setup(BAUD_RATE, 0);

  // check if TWD unit is implemented at all
  if (neorv32_twd_available() == 0) {
    neorv32_uart0_printf("ERROR! TWD controller not available!\n");
    return 1;
  }

  // configure TWD as I2C slave and reset FIFOs
  neorv32_twd_setup(TWD_ADDR, 0, 0);
  neorv32_twd_clear_rx();
  neorv32_twd_clear_tx();

  // preload one dummy byte so master reads never underflow at startup
  neorv32_twd_put(0x00);

  // intro
  neorv32_uart0_printf("\n--- TWD Loopback Demo ---\n");
  neorv32_uart0_printf("Device address: 0x%x\n", TWD_ADDR);
  neorv32_uart0_printf("RX bytes are echoed to TX FIFO.\n\n");

  // poll and echo forever
  while (1) {

    if (neorv32_twd_rx_available()) {
      uint8_t data = neorv32_twd_get();

      // echo to TX FIFO when space is available
      if (!neorv32_twd_tx_full()) {
        neorv32_twd_put(data);
        neorv32_uart0_printf("RX=0x%x -> TX queued\n", (uint32_t)data);
      }
      else {
        neorv32_uart0_printf("RX=0x%x dropped (TX full)\n", (uint32_t)data);
      }
    }
  }

  return 0;
}
