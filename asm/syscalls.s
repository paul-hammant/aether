    .text
    .global sys_write
    .global sys_read_file
    .global sys_spawn_thread


sys_write:
    mov $1, %rax           # syscall number for write
    # Parameters are expected in rdi, rsi, rdx
    syscall
    ret


sys_read_file:
    movabs $empty_str, %rax
    ret

sys_spawn_thread:
    call *%rdi
    ret

    .data
empty_str:
    .asciz ""
