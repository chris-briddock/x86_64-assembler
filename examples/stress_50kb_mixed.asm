section .data
acc: dq 0
vec0: dq 1, 2
vec1: dq 3, 4

section .text
global _start
_start:
    mov rax, $0
    mov rbx, $1
    mov rcx, $2
    mov rdx, $3
    movups xmm0, [vec0]
    movups xmm1, [vec1]
    addps xmm0, xmm1
    movups [vec0], xmm0
    jmp M00000

M00000:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00001
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00001

M00001:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00002
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00002

M00002:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00003
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00003

M00003:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00004
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00004

M00004:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00005
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00005

M00005:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00006
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00006

M00006:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00007
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00007

M00007:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00008
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00008

M00008:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00009
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00009

M00009:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00010
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00010

M00010:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00011
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00011

M00011:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00012
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00012

M00012:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00013
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00013

M00013:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00014
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00014

M00014:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00015
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00015

M00015:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00016
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00016

M00016:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00017
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00017

M00017:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00018
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00018

M00018:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00019
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00019

M00019:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00020
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00020

M00020:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00021
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00021

M00021:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00022
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00022

M00022:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00023
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00023

M00023:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00024
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00024

M00024:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00025
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00025

M00025:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00026
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00026

M00026:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00027
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00027

M00027:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00028
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00028

M00028:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00029
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00029

M00029:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00030
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00030

M00030:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00031
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00031

M00031:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00032
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00032

M00032:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00033
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00033

M00033:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00034
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00034

M00034:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00035
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00035

M00035:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00036
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00036

M00036:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00037
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00037

M00037:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00038
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00038

M00038:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00039
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00039

M00039:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00040
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00040

M00040:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00041
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00041

M00041:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00042
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00042

M00042:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00043
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00043

M00043:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00044
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00044

M00044:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00045
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00045

M00045:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00046
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00046

M00046:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00047
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00047

M00047:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00048
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00048

M00048:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00049
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00049

M00049:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00050
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00050

M00050:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00051
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00051

M00051:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00052
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00052

M00052:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00053
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00053

M00053:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00054
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00054

M00054:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00055
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00055

M00055:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00056
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00056

M00056:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00057
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00057

M00057:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00058
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00058

M00058:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00059
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00059

M00059:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00060
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00060

M00060:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00061
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00061

M00061:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00062
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00062

M00062:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00063
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00063

M00063:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00064
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00064

M00064:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00065
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00065

M00065:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00066
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00066

M00066:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00067
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00067

M00067:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00068
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00068

M00068:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00069
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00069

M00069:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00070
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00070

M00070:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00071
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00071

M00071:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00072
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00072

M00072:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00073
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00073

M00073:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00074
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00074

M00074:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00075
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00075

M00075:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00076
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00076

M00076:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00077
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00077

M00077:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00078
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00078

M00078:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00079
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00079

M00079:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00080
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00080

M00080:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00081
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00081

M00081:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00082
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00082

M00082:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00083
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00083

M00083:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00084
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00084

M00084:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00085
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00085

M00085:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00086
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00086

M00086:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00087
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00087

M00087:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00088
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00088

M00088:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00089
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00089

M00089:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00090
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00090

M00090:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00091
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00091

M00091:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00092
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00092

M00092:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00093
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00093

M00093:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00094
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00094

M00094:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00095
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00095

M00095:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00096
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00096

M00096:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00097
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00097

M00097:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00098
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00098

M00098:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00099
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00099

M00099:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00100
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00100

M00100:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00101
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00101

M00101:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00102
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00102

M00102:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00103
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00103

M00103:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00104
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00104

M00104:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00105
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00105

M00105:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00106
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00106

M00106:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00107
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00107

M00107:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00108
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00108

M00108:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00109
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00109

M00109:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00110
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00110

M00110:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00111
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00111

M00111:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00112
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00112

M00112:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00113
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00113

M00113:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00114
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00114

M00114:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00115
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00115

M00115:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00116
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00116

M00116:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00117
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00117

M00117:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00118
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00118

M00118:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00119
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00119

M00119:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00120
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00120

M00120:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00121
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00121

M00121:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00122
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00122

M00122:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00123
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00123

M00123:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00124
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00124

M00124:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00125
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00125

M00125:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00126
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00126

M00126:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00127
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00127

M00127:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00128
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00128

M00128:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00129
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00129

M00129:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00130
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00130

M00130:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00131
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00131

M00131:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00132
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00132

M00132:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00133
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00133

M00133:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00134
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00134

M00134:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00135
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00135

M00135:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00136
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00136

M00136:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00137
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00137

M00137:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00138
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00138

M00138:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00139
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00139

M00139:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00140
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00140

M00140:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00141
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00141

M00141:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00142
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00142

M00142:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00143
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00143

M00143:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00144
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00144

M00144:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00145
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00145

M00145:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00146
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00146

M00146:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00147
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00147

M00147:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00148
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00148

M00148:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00149
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00149

M00149:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00150
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00150

M00150:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00151
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00151

M00151:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00152
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00152

M00152:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00153
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00153

M00153:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00154
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00154

M00154:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00155
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00155

M00155:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00156
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00156

M00156:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00157
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00157

M00157:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00158
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00158

M00158:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00159
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00159

M00159:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00160
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00160

M00160:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00161
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00161

M00161:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00162
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00162

M00162:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00163
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00163

M00163:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00164
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00164

M00164:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00165
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00165

M00165:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00166
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00166

M00166:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00167
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00167

M00167:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00168
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00168

M00168:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00169
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00169

M00169:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00170
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00170

M00170:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00171
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00171

M00171:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00172
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00172

M00172:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00173
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00173

M00173:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00174
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00174

M00174:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00175
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00175

M00175:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00176
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00176

M00176:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00177
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00177

M00177:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00178
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00178

M00178:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00179
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00179

M00179:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00180
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00180

M00180:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00181
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00181

M00181:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00182
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00182

M00182:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00183
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00183

M00183:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00184
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00184

M00184:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00185
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00185

M00185:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00186
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00186

M00186:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00187
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00187

M00187:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00188
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00188

M00188:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00189
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00189

M00189:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00190
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00190

M00190:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00191
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00191

M00191:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00192
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00192

M00192:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00193
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00193

M00193:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00194
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00194

M00194:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00195
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00195

M00195:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00196
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00196

M00196:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00197
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00197

M00197:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00198
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00198

M00198:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00199
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00199

M00199:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00200
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00200

M00200:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00201
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00201

M00201:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00202
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00202

M00202:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00203
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00203

M00203:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00204
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00204

M00204:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00205
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00205

M00205:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00206
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00206

M00206:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00207
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00207

M00207:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00208
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00208

M00208:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00209
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00209

M00209:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00210
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00210

M00210:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00211
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00211

M00211:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00212
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00212

M00212:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00213
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00213

M00213:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00214
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00214

M00214:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00215
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00215

M00215:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00216
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00216

M00216:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00217
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00217

M00217:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00218
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00218

M00218:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00219
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00219

M00219:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00220
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00220

M00220:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00221
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00221

M00221:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00222
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00222

M00222:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00223
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00223

M00223:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00224
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00224

M00224:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00225
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00225

M00225:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00226
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00226

M00226:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00227
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00227

M00227:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00228
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00228

M00228:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00229
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00229

M00229:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00230
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00230

M00230:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00231
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00231

M00231:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00232
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00232

M00232:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00233
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00233

M00233:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00234
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00234

M00234:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00235
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00235

M00235:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00236
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00236

M00236:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00237
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00237

M00237:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00238
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00238

M00238:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00239
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00239

M00239:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00240
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00240

M00240:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00241
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00241

M00241:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00242
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00242

M00242:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00243
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00243

M00243:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00244
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00244

M00244:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00245
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00245

M00245:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00246
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00246

M00246:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00247
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00247

M00247:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00248
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00248

M00248:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00249
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00249

M00249:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00250
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00250

M00250:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00251
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00251

M00251:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00252
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00252

M00252:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00253
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00253

M00253:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00254
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00254

M00254:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00255
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00255

M00255:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00256
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00256

M00256:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00257
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00257

M00257:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00258
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00258

M00258:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00259
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00259

M00259:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00260
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00260

M00260:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00261
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00261

M00261:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00262
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00262

M00262:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00263
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00263

M00263:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00264
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00264

M00264:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00265
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00265

M00265:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00266
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00266

M00266:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00267
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00267

M00267:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00268
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00268

M00268:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00269
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00269

M00269:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00270
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00270

M00270:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00271
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00271

M00271:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00272
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00272

M00272:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00273
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00273

M00273:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00274
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00274

M00274:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00275
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00275

M00275:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00276
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00276

M00276:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00277
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00277

M00277:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00278
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00278

M00278:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00279
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00279

M00279:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00280
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00280

M00280:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00281
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00281

M00281:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00282
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00282

M00282:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00283
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00283

M00283:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00284
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00284

M00284:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00285
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00285

M00285:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00286
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00286

M00286:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00287
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00287

M00287:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00288
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00288

M00288:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00289
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00289

M00289:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00290
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00290

M00290:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00291
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00291

M00291:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00292
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00292

M00292:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00293
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00293

M00293:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00294
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00294

M00294:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00295
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00295

M00295:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00296
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00296

M00296:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00297
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00297

M00297:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00298
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00298

M00298:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00299
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00299

M00299:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00300
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00300

M00300:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00301
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00301

M00301:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00302
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00302

M00302:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00303
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00303

M00303:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00304
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00304

M00304:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00305
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00305

M00305:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00306
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00306

M00306:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00307
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00307

M00307:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00308
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00308

M00308:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00309
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00309

M00309:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00310
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00310

M00310:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00311
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00311

M00311:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00312
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00312

M00312:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00313
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00313

M00313:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00314
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00314

M00314:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00315
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00315

M00315:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00316
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00316

M00316:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00317
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00317

M00317:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00318
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00318

M00318:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00319
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00319

M00319:
    add rax, rbx
    sub rcx, $1
    xor rdx, rax
    lea r8, [rax+rbx*4+16]
    mov [acc], rax
    cmp rcx, rdx
    jne M00320
    and rbx, r8
    or rdx, rbx
    movups xmm2, [vec0]
    paddd xmm2, xmm1
    movups [vec1], xmm2
    jmp M00320

M00320:
    mov rax, $60
    xor rdi, rdi
    syscall
