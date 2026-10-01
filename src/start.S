/* Freestanding aarch64 entry point for jbm_menu.
 *
 * The kernel hands control here with sp pointing at:
 *   [sp]      argc
 *   [sp + 8]  argv[0..argc-1], NULL
 *             envp[0..], NULL
 *             auxv
 * We normalise the stack, build argc/argv/envp and call main. main never
 * returns; if it somehow does we park the CPU.
 */

    .section .text.startup._start, "ax", @progbits
    .globl _start
    .type _start, %function
    .balign 4
_start:
    mov     x29, #0
    mov     x30, #0
    ldr     x19, [sp]            /* argc */
    add     x20, sp, #8          /* argv */
    add     x21, x20, #8         /* envp = argv + (argc + 1) */
    add     x21, x21, x19, lsl #3
    mov     x0, x19
    mov     x1, x20
    mov     x2, x21
    bl      jbm_main
.Lhang:
    wfe
    b       .Lhang
    .size _start, . - _start
