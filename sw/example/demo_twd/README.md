# NEORV32 demo_twd

`demo_twd` configures the NEORV32 TWD peripheral as an I2C device at address
`0x32`. Received bytes are read from the TWD RX FIFO and echoed back into the
TX FIFO, which makes the example useful for QEMU host-driven I2C validation.

Build the payload with the RV32 bare-metal toolchain:

```bash
cd sw/example/demo_twd
PATH=/opt/riscv32-unknown-elf.gcc-12.1.0/bin:$PATH \
  RISCV_PREFIX=riscv32-unknown-elf- \
  make clean_all all
```

For QEMU regression testing, rebuild the bootloader from the same NEORV32 tree
and run the external test from `qemu_neorv32_extras`:

```bash
cd sw/bootloader
PATH=/opt/riscv32-unknown-elf.gcc-12.1.0/bin:$PATH \
  RISCV_PREFIX=riscv32-unknown-elf- \
  make clean_all all \
  MARCH=rv32imc_zicsr_zifencei \
  MABI=ilp32 \
  USER_FLAGS="-Wl,--defsym,__neorv32_ram_size=256 -Wl,--defsym,__neorv32_ram_base=0x80000000 -Wl,--defsym,__neorv32_rom_size=16k -Wl,--defsym,__neorv32_rom_base=0xFFE00000 -DMAKE_BOOTLOADER -flto"

cd /home/smishash/shonot/sources/qemu_neorv32_extras
QEMU_RISCV32_BIN=/home/smishash/shonot/sources/qemu/build/qemu-system-riscv32 \
NEORV32_DEMO_TWD_BIN=/home/smishash/shonot/fpga_projects/neorv32/sw/example/demo_twd/neorv32_exe.bin \
NEORV32_BOOTLOADER_BIN=/home/smishash/shonot/fpga_projects/neorv32/sw/bootloader/neorv32_raw_exe.bin \
python3 tests/unit/test_twi_i2c_slave.py
```

Expected result:

```text
Responsive I2C address: 0x32
Write reply: OK
Echo read reply: D 11 22 33 ff ff ff ff ff
test_twi_i2c_slave PASSED
```