# Project Specific
NAME := flappy
SRCS = src/main.c src/bmp.c

# Environment Specific
include env.mk

# Colors
ifeq ($(strip $(wildcard colors.mk)),)
	CLR_CC  = ""
	CLR_LD  = ""
	CLR_OBJ = ""
	CLR_CLN = ""
	CLR_RS   = ""
else
	include colors.mk
endif

# Tools
CC := $(PREFIX)$(GCC_PREFIX)gcc
LD := $(PREFIX)$(GCC_PREFIX)ld
OBJCOPY := $(PREFIX)$(GCC_PREFIX)objcopy
QEMU := qemu-system-x86_64
WGET := wget
TAR := tar

# Build Flags
CFLAGS = -fpic -ffreestanding -fno-stack-protector -fno-stack-check -fshort-wchar -mno-red-zone -maccumulate-outgoing-args
INCDIR = -I$(GNU_EFI)/inc -I$(GNU_EFI)/inc/$(EFI_ARCH)
LDFLAGS = -shared -Bsymbolic -L$(GNU_EFI)/$(EFI_ARCH)/lib -L$(GNU_EFI)/$(EFI_ARCH)/gnuefi -T$(GNU_EFI)/gnuefi/elf_$(EFI_ARCH)_efi.lds
LDLIBS = -lgnuefi -lefi

# OVMF Info
OVMF_URI := https://github.com/rust-osdev/ovmf-prebuilt/releases/download/edk2-stable202511-r1/edk2-stable202511-r1-bin.tar.xz
OVMF_SHA256 := 79841c5dcac6d4bb71ead5edb6ca2a251237330be3c0b166bdc8a8fec0ce760d
OVMF_OUT := ovmf.tar.xz
OVMF_EXTRACTED_FOLDER := edk2-stable202511-r1-bin

# Targets
efi: $(NAME).efi
	$(info Compiled $(NAME).efi!)

.PHONY: checkenv
checkenv:
	$(info Environment Details)
	$(info )
	$(info CC: $(CC))
	$(info LD: $(LD))
	$(info OBJCOPY: $(OBJCOPY))
	$(info WGET: $(WGET))
	$(info TAR: $(TAR))
	$(info )
	$(info GNU_EFI: $(GNU_EFI))
	$(info EFI_ARCH: $(EFI_ARCH))
	$(info OVMF_ARCH: $(OVMF_ARCH))
	$(info BOOT_ARCH: $(BOOT_ARCH))
	$(info )
	$(info INCS: $(INCDIR))
	$(info LIBS: $(LDLIBS))
	$(info )
	$(info OVMF_ARC: $(OVMF_OUT))
	$(info OVMF_DIR: $(OVMF_DIR))

.PHONY: clean
clean:
	@printf '$(CLR_CC)CLN$(CLR_RS) object files\n'
	@rm -f $(SRCS:%.c=%.o)
	@printf '$(CLR_CC)CLN$(CLR_RS) dynamic library\n'
	@rm -f $(NAME).so
	@printf '$(CLR_CC)CLN$(CLR_RS) EFI file\n'
	@rm -f $(NAME).efi
	@printf '$(CLR_CC)CLN$(CLR_RS) EFI System Partition\n'
	@rm -rf ./esp/
	@printf '$(CLR_CC)CLN$(CLR_RS) OVMF\n'
	@rm -f ovmf.tar.xz
	@rm -rf ./ovmf/

.PHONY: qemu
qemu: efi $(ESP_DIR)
	@cp $(NAME).efi $(ESP_DIR)
	$(QEMU) -drive if=pflash,format=raw,readonly=on,file=$(OVMF_DIR)/$(OVMF_ARCH)/code.fd -drive if=pflash,format=raw,file=$(OVMF_DIR)/$(OVMF_ARCH)/vars.fd -drive file=fat:rw:$(abspath $(ESP_DIR)),format=raw -serial stdio

%.o: %.c
	@printf '$(CLR_CC)CC $(CLR_RS) $@\n'
	@$(CC) $(CFLAGS) -c $< -o $@ $(INCDIR)

$(NAME).so: $(SRCS:%.c=%.o)
	@printf '$(CLR_LD)LD $(CLR_RS) $(NAME).so\n'
	@$(LD) $(LDFLAGS) $(GNU_EFI)/$(EFI_ARCH)/gnuefi/crt0-efi-$(EFI_ARCH).o $(SRCS:%.c=%.o) -o $(NAME).so $(LDLIBS)

$(NAME).efi: $(NAME).so
	@printf '$(CLR_OBJ)OBJ$(CLR_RS) $(NAME).efi\n'
	@$(OBJCOPY) -j .text -j .sdata -j .data -j .rodata -j .dynamic -j .dynsym -j .rel -j .rela -j .rel.* -j .rela.* -j .reloc --output-target efi-app-x86_64 --subsystem=10 $(NAME).so $(NAME).efi

$(OVMF_OUT): 
	$(info Downloading OVMF...)
	$(WGET) -q --show-progress $(OVMF_URI) -O $(OVMF_OUT)
	@ echo "$(OVMF_SHA256) $(OVMF_OUT)" | sha256sum --check

$(OVMF_DIR): $(OVMF_OUT)
	$(TAR) -xvf $(OVMF_OUT)
	@mv $(OVMF_EXTRACTED_FOLDER) $(OVMF_DIR)/

$(ESP_DIR): $(OVMF_DIR)
	@mkdir -p $(ESP_DIR)/EFI/BOOT
	@cp $(OVMF_DIR)/$(OVMF_ARCH)/shell.efi $(ESP_DIR)/EFI/BOOT/BOOT$(BOOT_ARCH).EFI
	@cp startup.nsh $(ESP_DIR)
	@mkdir -p $(ESP_DIR)/flappyres
	@cp -r res/* $(ESP_DIR)/flappyres
