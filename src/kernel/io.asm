[bits 32]

section .text

global inb ;将inb函数导出，给其他模块使用
inb:
  push ebp
  mov ebp, esp

  xor eax, eax
  mov edx, [ebp + 8] ;上一个函数传入的第一个参数
  in al, dx

  nop
  nop
  nop
  leave
  ret

global outb
outb:
  push ebp
  mov ebp, esp
  mov edx, [ebp + 8]  ; 1. port
  mov eax, [ebp + 12] ; 2. value
  out dx, al
  
  nop
  nop
  nop
  leave
  ret

global inw ;将inw函数导出，给其他模块使用
inw:
  push ebp
  mov ebp, esp

  xor eax, eax
  mov edx, [ebp + 8] ;上一个函数传入的第一个参数
  in ax, dx

  nop
  nop
  nop
  leave
  ret

global outw
outw:
  push ebp
  mov ebp, esp
  mov edx, [ebp + 8]  ; 1. port
  mov eax, [ebp + 12] ; 2. value
  out dx, ax
  
  nop
  nop
  nop
  leave
  ret