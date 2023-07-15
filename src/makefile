BUILD:= ../build
SRC:=.
TESTS:=../tests
MULTIBOOT2:=0x10000
ENTRYPOINT:=0x10040

CFLAGS:= -m32 # 32 位的程序
CFLAGS+= -fno-builtin	# 不需要 gcc 内置函数
CFLAGS+= -nostdinc		# 不需要标准头文件
CFLAGS+= -fno-pic		# 不需要位置无关的代码  position independent code
CFLAGS+= -fno-pie		# 不需要位置无关的可执行程序 position independent executable
CFLAGS+= -nostdlib		# 不需要标准库
CFLAGS+= -fno-stack-protector	# 不需要栈保护
CFLAGS:=$(strip ${CFLAGS})

DEBUG:= -g
INCLUDE:=-I$(SRC)/include

$(BUILD)/boot/%.bin: $(SRC)/boot/%.asm
	$(shell mkdir -p $(dir $@))
	nasm -f bin $< -o $@

$(BUILD)/kernel/%.o: $(SRC)/kernel/%.asm
	$(shell mkdir -p $(dir $@))
	nasm -f elf32 -gdwarf $< -o $@

$(BUILD)/kernel/%.o: $(SRC)/kernel/%.c
	$(shell mkdir -p $(dir $@))
	gcc $(CFLAGS) $(DEBUG) $(INCLUDE) -c $< -o $@

$(BUILD)/lib/%.o: $(SRC)/lib/%.c
	$(shell mkdir -p $(dir $@))
	gcc $(CFLAGS) $(DEBUG) $(INCLUDE) -c $< -o $@

LDFLAGS:= -m elf_i386 \
		-static \
		-Ttext $(ENTRYPOINT)\
		--section-start=.multiboot2=$(MULTIBOOT2)
LDFLAGS:=$(strip ${LDFLAGS})


$(BUILD)/kernel.bin: $(BUILD)/kernel/start.o \
										 $(BUILD)/kernel/main.o	\
										 $(BUILD)/kernel/io.o \
										 $(BUILD)/lib/string.o \
										 $(BUILD)/kernel/console.o \
										 $(BUILD)/lib/vsprintf.o \
										 $(BUILD)/kernel/printk.o \
										 $(BUILD)/kernel/assert.o \
										 $(BUILD)/kernel/task.o \
										 $(BUILD)/kernel/schedule.o \
										 $(BUILD)/kernel/debug.o \
										 $(BUILD)/kernel/global.o \
										 $(BUILD)/kernel/handler.o \
										 $(BUILD)/kernel/interrupt.o \
										 $(BUILD)/kernel/clock.o \
										 $(BUILD)/kernel/time.o \
										 $(BUILD)/kernel/rtc.o \
										 $(BUILD)/kernel/memory.o \
										 $(BUILD)/lib/stdlib.o\
										 $(BUILD)/lib/bitmap.o\
										 $(BUILD)/kernel/gate.o \
										 $(BUILD)/lib/syscall.o\
										 $(BUILD)/lib/list.o\
										 $(BUILD)/kernel/thread.o \
										 $(BUILD)/kernel/mutex.o \
										 $(BUILD)/kernel/keyboard.o \
										 
	$(shell mkdir -p $(dir $@))
	ld ${LDFLAGS} $^ -o $@ 

$(BUILD)/system.bin: $(BUILD)/kernel.bin
	objcopy -O binary $< $@

$(BUILD)/system.map: $(BUILD)/kernel.bin
	nm $< | sort > $@

$(BUILD)/master.img: $(BUILD)/boot/boot.bin \
										 $(BUILD)/boot/loader.bin \
										 $(BUILD)/system.bin \
										 $(BUILD)/system.map
	dd if=$(BUILD)/boot/boot.bin of=$@ bs=512 count=1 conv=notrunc
	dd if=$(BUILD)/boot/loader.bin of=$@ bs=512 count=4 seek=2 conv=notrunc
	dd if=$(BUILD)/system.bin of=$@ bs=512 count=200 seek=10 conv=notrunc

$(BUILD)/kernel.iso : $(BUILD)/kernel.bin $(SRC)/utils/grub.cfg

# 检测内核文件是否合法
	grub-file --is-x86-multiboot2 $<
# 创建 iso 目录
	mkdir -p $(BUILD)/iso/boot/grub
# 拷贝内核文件
	cp $< $(BUILD)/iso/boot
# 拷贝 grub 配置文件
	cp $(SRC)/utils/grub.cfg $(BUILD)/iso/boot/grub
# 生成 iso 文件
	grub-mkrescue -o $@ $(BUILD)/iso

test : $(BUILD)/master.img

.PHONY: clean
clean:
	find $(BUILD) ! -name 'master.img' -type f -exec rm -rf {} +
	rm -rf $(TESTS)/*.out
.PHONY: bochs
bochs: $(BUILD)/master.img
	bochs -q -f ../bochs/bochsrc


QEMU:= qemu-system-i386 \
	-m 32M \
	-rtc base=localtime \

QEMU_DISK:=-boot c \
	-drive file=$(BUILD)/master.img,if=ide,index=0,media=disk,format=raw \

QEMU_CDROM:=-boot d \
	-drive file=$(BUILD)/kernel.iso,media=cdrom \

QEMU_DEBUG:= -s -S

.PHONY: qemu-gui
qemu-gui: $(BUILD)/master.img
	$(QEMU) $(QEMU_DISK)

.PHONY: qemu-gui-cd
qemu-gui-cd: $(BUILD)/kernel.iso
	$(QEMU) $(QEMU_CDROM)

.PHONY: qemug-gui
qemu-gui-debug: $(BUILD)/master.img
	qemu-system-i386 \
	-s -S \
	-m 132M \
	-boot c \
	-hda $<

.PHONY: qemu
qemu: $(BUILD)/master.img
	qemu-system-i386 \
	-m 32M \
	-boot c \
	-nographic \
	-hda $<

.PHONY: qemug
qemu-debug: $(BUILD)/master.img
	qemu-system-i386 \
	-s -S \
	-m 32M \
	-boot c \
	-nographic \
	-hda $<