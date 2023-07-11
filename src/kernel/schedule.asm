[bits 32]
section .text


global task_switch
task_switch:

    ; 当前任务会把ABI要求的寄存器保存在当前任务栈
    ; 当再次调用此任务时，会从栈中弹出
    push ebp
    mov ebp, esp

    push ebx
    push esi
    push edi

    mov eax, esp;
    ; 获取当前栈底，也就是task_t的地址
    and eax, 0xfffff000; current

    ; 将当前栈指针保存到task_t中
    mov [eax], esp

    ; 获取调用参数，也就是下一个栈的task_t
    mov eax, [ebp + 8]; next
    ; 获取下一个栈的task_t的esp
    mov esp, [eax]

    ; 此刻开始，所有的操作就是在下一个任务栈
    ; 依次弹栈，恢复下个任务栈的环境
    pop edi
    pop esi
    pop ebx
    pop ebp
    ; 将新的返回地址给 eip
    ret
