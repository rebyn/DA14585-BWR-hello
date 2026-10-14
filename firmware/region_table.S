/* Keil scatter compat, GNU AS: emulate Region$$Table$$Base so the UNMODIFIED
 * ancestor sources link. Ancestor reads *(u32*)(&Base + 0x10) as firmware end;
 * word[4] is therefore __bss_start__ (first address past .data in the SDK
 * ldscript). VMA==LMA throughout (execute-in-RAM), so no load offset applies.
 */
    .syntax unified
    .arch armv6-m
    .section .region_table,"a",%progbits
    .align 2
    .global Region$$Table$$Base
    .type Region$$Table$$Base, %object
Region$$Table$$Base:
    .word 0
    .word 0
    .word 0
    .word 0
    .word __bss_start__
    .size Region$$Table$$Base, .-Region$$Table$$Base
