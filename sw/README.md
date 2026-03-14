## Software Framework

This folder provides the hardware abstraction layer, drivers, scripts
and helpers of the NEORV32 software framework.

### > [`bootloader`](bootloader)

Source(s) of the default NEORV32 bootloader.
A pre-built image is already installed into the rtl design via the
`rtl/core/neorv32_bootloader_image.vhd` file.

### > [`common`](common)

NEORV32-specific common files: central application Makefile,
linker script and processor start-up code.

### > [`example`](example)

Several example/demo programs for testing and for getting started.

For external QEMU-based TWD/I2C slave tests that use `sw/example/demo_twd`,
build the executable with an explicit toolchain prefix if your environment
does not provide `riscv-none-elf-*` binaries:

```bash
cd sw/example/demo_twd
make clean all RISCV_PREFIX=riscv32-unknown-elf-
```

### > [`image_gen`](image_gen)

Helper program to generate NEORV32 executables (for upload via bootloader) and VHDL
memory initialization files. This program is automatically compiled when
executing one of the application compilation makefile targets.

### > [`lib`](lib)

Core libraries (sources and header files) and helper functions for using
the processor peripherals and the CPU itself.

### > [`ocd-firmware`](ocd-firmware)

Firmware (debugger "park loop") for the on-chip debugger.

### > [`openocd`](openocd)

Configuration files for openOCD to connect to the NEORV32 on-chip debugger via JTAG.

### > [`svd`](svd)

Contains a CMSIS-SVD compatible system view description file for all
default processor peripherals.
