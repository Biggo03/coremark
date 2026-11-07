# Copyright 2018 Embedded Microprocessor Benchmark Consortium (EEMBC)
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# Original Author: Shay Gal-on

# File : core_portme.mak

OUTFLAG = -o

RISCV_PREFIX ?= riscv64-unknown-elf
CC       = $(RISCV_PREFIX)-gcc
LD       = $(CC)
OBJCOPY  = $(RISCV_PREFIX)-objcopy
OBJDUMP  = $(RISCV_PREFIX)-objdump

ARCH = rv32i_zicsr
ABI  = ilp32

ITERATIONS ?= 1

PORT_CFLAGS = -O2 -march=$(ARCH) -mabi=$(ABI) -ffreestanding -fno-builtin -fno-common -fno-delete-null-pointer-checks -fno-isolate-erroneous-paths-dereference -msmall-data-limit=8
FLAGS_STR   = "$(PORT_CFLAGS) $(XCFLAGS) $(XLFLAGS) $(LFLAGS_END)"
CFLAGS      = $(PORT_CFLAGS) -I$(PORT_DIR) -I. -DFLAGS_STR=\"$(FLAGS_STR)\"

SEPARATE_COMPILE = 1
OBJOUT  = -o
LFLAGS  = -march=$(ARCH) -mabi=$(ABI)
ASFLAGS =
OFLAG   = -o
COUT    = -c

LFLAGS_END = -T $(PORT_DIR)/linker.ld -nostdlib -Wl,-Map,$(OUTFILE:.elf=.map) -lgcc

PORT_SRCS = $(PORT_DIR)/core_portme.c $(PORT_DIR)/_start.s
PORT_OBJS = $(PORT_DIR)/core_portme$(OEXT) $(PORT_DIR)/_start$(OEXT)
vpath %.c $(PORT_DIR)
vpath %.s $(PORT_DIR)

LOAD = echo "Load $(OUTFILE:.elf=.text.hex) into IMEM and $(OUTFILE:.elf=.data.hex) into DMEM."
RUN  = echo "Use the riscv_pipelined simulation flow to execute $(OUTFILE)."

OEXT = .o
EXE  = .elf

PORT_CLEAN = $(OUTFILE:.elf=.dump) $(OUTFILE:.elf=.map) $(OUTFILE:.elf=.text.hex) $(OUTFILE:.elf=.data.hex) $(OUTFILE:.elf=.text.bin) $(OUTFILE:.elf=.data.bin)

$(OPATH)$(PORT_DIR)/%$(OEXT) : %.c
	$(CC) $(CFLAGS) $(XCFLAGS) $(COUT) $< $(OBJOUT) $@

$(OPATH)%$(OEXT) : %.c
	$(CC) $(CFLAGS) $(XCFLAGS) $(COUT) $< $(OBJOUT) $@

$(OPATH)$(PORT_DIR)/%$(OEXT) : %.s
	$(CC) $(PORT_CFLAGS) $(XCFLAGS) $(COUT) $< $(OBJOUT) $@

.PHONY : port_prebuild port_postbuild port_prerun port_postrun port_preload port_postload
port_pre% :

port_postbuild :
	$(OBJDUMP) -D $(OUTFILE) > $(OUTFILE:.elf=.dump)
	$(OBJCOPY) -O binary -j .text -j .init $(OUTFILE) $(OUTFILE:.elf=.text.bin)
	hexdump -v -e '1/4 "%08x\n"' $(OUTFILE:.elf=.text.bin) > $(OUTFILE:.elf=.text.hex)
	rm -f $(OUTFILE:.elf=.text.bin)
	$(OBJCOPY) -O binary -j .rodata -j .data -j .sdata $(OUTFILE) $(OUTFILE:.elf=.data.bin)
	if [ -s $(OUTFILE:.elf=.data.bin) ]; then hexdump -v -e '1/4 "%08x\n"' $(OUTFILE:.elf=.data.bin) > $(OUTFILE:.elf=.data.hex); else : > $(OUTFILE:.elf=.data.hex); fi
	rm -f $(OUTFILE:.elf=.data.bin)

port_prerun port_postrun port_preload port_postload :

OPATH = ./
MKDIR = mkdir -p
