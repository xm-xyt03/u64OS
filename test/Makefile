IMAGE = kernel.iso
ARCH = x86_64
FIRMWARE = $(ARCH)-efi
BIOS = -bios /usr/share/ovmf/OVMF.fd
RM = rm -rf

$(IMAGE): target/$(ARCH)/iso/boot/kernel.bin
	@grub-mkrescue /usr/lib/grub/$(FIRMWARE) -o $@ target/$(ARCH)/iso

run: $(IMAGE)
	@qemu-system-$(ARCH) \
		$(BIOS) \
		-name "u64OS Running Environment" \
		-cpu kvm64,+smep,+smap \
		-smp sockets=1,dies=1,cores=4,threads=2 \
		-m 4G \
		--machine q35 \
		-drive file=$<,format=raw,index=0,media=disk \
		-serial stdio

clean:
	@$(RM) kernel.iso
