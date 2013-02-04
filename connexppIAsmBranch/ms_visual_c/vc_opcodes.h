
    /* 9-bit opcodes (instruction will NOT have immediate value) */
    #define _ADD     0x144
    #define _ADDC    0x164
    #define _SUB     0x154
    #define _SUBC    0x174

    #define _NOT    0x14C 
    #define _OR     0x15C
    #define _AND    0x16C
    #define _XOR    0x17C
    #define _EQ     0x148
    #define _LT     0x158
    #define _ULT    0x168
    #define _SHL    0x140
    #define _SHR    0x150
    #define _SHRA   0x160 
    #define _ISHL   0x141
    #define _ISHR   0x151
    #define _ISHRA  0x161

    #define _LDIX    0x120
    #define _LDSH    0x130
    #define _CELL_SHL 0x112
    #define _CELL_SHR 0x111

    #define _READ    0x124
    #define _WRITE   0x114

    #define _MULT    0x108
    #define _MULT_LO   0x128
    #define _MULT_HI   0x138

    #define _WHERE_CRY 0x11C
    #define _WHERE_EQ  0x11D
    #define _WHERE_LT  0x11E
    #define _END_WHERE 0x11F
    #define _REDUCE    0x100
    #define _NOP       0x00

    /* 6-bit opcodes (instruction will have immediate value) */
    #define _VLOAD     0x35
    #define _IREAD     0x34
    #define _IWRITE    0x32