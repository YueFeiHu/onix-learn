
boot.bin: boot.asm
	nasm -f bin boot.asm -o boot.bin

master.img: boot.bin
	yes | bximage -q
	dd if=boot.bin of=master.img bs=512 count=1 conv=notrunc

.PHONY: clean
clean:
	rm -rf *.bin
	rm -rf *.img

.PHONY: bochs
bochs: master.img
	bochs -q
