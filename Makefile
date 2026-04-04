BUILD_DIR = build
ARCH = x86_64
TARGET_DIR = test/target/x86_64/iso/

.PHONY: all clean

all: $(TARGET_DIR)/boot/kernel.bin
	@$(MAKE) -C test --no-print-directory

$(BUILD_DIR)/kernel.bin:
	@(cd src/arch/x86/boot/ && objcopy -O elf64-x86-64 -B i386 -I binary font.psf boot_font.o)
	@cmake -S src/ -B $(BUILD_DIR)
	@$(MAKE) -C $(BUILD_DIR)

$(TARGET_DIR)/boot/kernel.bin: $(BUILD_DIR)/kernel.bin
	@cp $^ $(TARGET_DIR)/boot/
	
clean:
	@rm -rf $(BUILD_DIR)
	@rm -rf $(TARGET_DIR)/boot/kernel.bin
	@rm -rf src/arch/x86/boot/boot_font.o
	@$(MAKE) -C test clean --no-print-directory

run: $(TARGET_DIR)/boot/kernel.bin
	@$(MAKE) -C test run --no-print-directory