; 本代码段从0x7c00开始，由BIOS把本段代码加载到该地址
[org 0x7c00]

; 设置屏幕模式为文本模式，清除屏幕
; 这两句不加，会卡在boot
mov ax, 3
int 0x10

; 初始化段寄存器为0
mov ax, 0
mov ds, ax
mov es, ax
mov ss, ax
; 将栈指针设为0x7c00, 像下增长，代码段向上增长
mov sp, 0x7c00

;bochs 魔术断点，使程序停在此处
xchg bx, bx;

mov si, booting_str
call print

; 阻塞
jmp $

print:
  mov ah, 0x0e
.next:
  mov al, [si]
  cmp al, 0
  jz .done
  int 0x10
  inc si
  jmp .next
.done:
  ret

booting_str:
  db "hello,world...",10,13,0 ; \n\r

; 填充 0
; $ 表示当前指令地址，$$ 表示此section开始地址
times 510 - ($ - $$) db 0

; 主引导扇区的最后两个字节必须是 0x55 0xaa
; dw 0xaa55 小端法
db 0x55, 0xaa