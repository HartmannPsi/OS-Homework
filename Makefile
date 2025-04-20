# $ vim ~/kvm/Makefile

.PHONY: initramfs run-win run-console run-ubuntu

initramfs:
	cd busybox-1.36.1/_install/ && find . -print0 | cpio --null -ov --format=newc | gzip -9 > ../initramfs.cpio.gz
        
run-win:
	qemu-system-x86_64 \
		-kernel ./linux-5.19.17/arch/x86/boot/bzImage \
		-initrd ./busybox-1.36.1/initramfs.cpio.gz \
		-append "init=/init"
        
run-console:
	qemu-system-x86_64 \
		-kernel ./linux-5.19.17/arch/x86/boot/bzImage \
		-initrd ./busybox-1.36.1/initramfs.cpio.gz \
		-nographic \
		-append "init=/init console=ttyS0"

run-ubuntu:
	sudo qemu-system-x86_64 -m 4096 -cdrom ./ubuntu-24.04.2-desktop-amd64.iso -bios ./edk2/Build/OvmfX64/DEBUG_GCC5/FV/OVMF.fd