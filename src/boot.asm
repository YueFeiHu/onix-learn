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

mov edi, 0x1000 ;读取硬盘块到目标内存
mov ecx, 2      ;起始扇区
mov bl, 4       ;扇区数量

call read_disk

cmp word [0x1000], 0x55aa
jnz error

jmp 0:0x1002


; 阻塞
jmp $

read_disk:
  mov dx, 0x1f2
  mov al, bl    ;out指令只接受al和dx寄存器
  out dx, al    ;将al内容写到dx代表的设备寄存器中

  inc dx  ;0x1f3
  mov al, cl
  out dx, al

  inc dx  ;0x1f4
  shr ecx, 8
  mov al, cl
  out dx, al

  inc dx  ;0x1f5
  shr ecx, 8
  mov al, cl
  out dx, al

  inc dx  ;0x1f6
  shr ecx, 8
  and cl, 0b1111

  mov al, 0b1110_0000
  or al, cl
  out dx, al

  inc dx  ;0x1f7
  mov al, 0x20
  out dx, al

  xor ecx, ecx
  mov cl, bl    ;得到读写扇区的数量

  .read:
    push cx
    call .waits ;等待数据准备完成
    call .reads ;读取一个扇区
    pop cx
    loop .read
  ret

  .waits:
    mov dx, 0x1f7
    .check:
      in al, dx ;从0x1f7端口读取状态   
      nop
      nop
      nop
      and al, 0b1000_1000
      cmp al, 0b0000_1000
      jnz .check
    ret
  
  .reads:
    mov dx, 0x1f0
    mov cx, 256
    .readw:
      in ax, dx ;从0x1f0读取一个字
      nop
      nop
      nop
      mov [edi], ax
      add edi, 2
      loop .readw
    ret

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

error:
    mov si, .msg
    call print
    hlt; 让 CPU 停止
    jmp $
    .msg db "Booting Error!!!", 10, 13, 0

; 填充 0
; $ 表示当前指令地址，$$ 表示此section开始地址
times 510 - ($ - $$) db 0

; 主引导扇区的最后两个字节必须是 0x55 0xaa
; dw 0xaa55 小端法
db 0x55, 0xaa