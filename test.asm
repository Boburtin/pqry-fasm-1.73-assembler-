format PE64 Console 6.0
entry start

section ".text" readable executable
start:  push rbp
        mov rsp, rbp
        sub rsp, 40 + 8 
        add rsp, 40 + 8
        pop rbp
        mov rax, 2
        ret

section ".data" data readable writeable
str: db "Hello",0
str2: db "World",10,0
