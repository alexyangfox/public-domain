/* This is free and unencumbered software released into the public domain. */

.section .text

.global __sys_exit
__sys_exit:
  movq $60, %rax /* sys_exit */
  syscall
  hlt
