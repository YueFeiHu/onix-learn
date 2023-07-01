[bits 32]

global _start
_start:
    mov byte [0xb8000], 0x55; 表示进入了内核
    jmp $; 阻塞

msg:
    db "hello,world", 10, 13, 0