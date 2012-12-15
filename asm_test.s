.globl main
nop
iwr r4 100
ird r10 0x32
vload r0 0x140
red r3
mult r1 r2
cellshr r2 r3
cellshl r3 r5
write r7 r1
wherecry
whereeq
wherelt
endwhere
ldix r1
read r3 r6
multl r3
ldsh r15
multh r31
shl r31 r30 r29
ishl r14 r20 0x8
add r0 r1 r2
eq r3 r4 r5
lnot r3 r1
shr r1 r2 r3
ishr r3 r5 3
sub r3 r3 r3
lt r3 r4 r5
lor r1 r1 r1
shra r3 r3 r3
ishra r3 r4 9
addc r1 r2 r4
ult r4 r3 r2
land r6 r5 r10
subc r4 r4 r4
lxor r1 r1 r1
