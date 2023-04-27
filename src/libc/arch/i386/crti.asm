bits 32

    ;; this file is entierlythis file is entierly  linker magic

section .init
global _init
_init:
    push ebp
    mov ebp, esp

section .fini
global _fini
_fini:
    push ebp
    mov ebp, esp
