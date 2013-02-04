
/*
 *
 * File: aes_tests.cpp
 *
 * AES tests.
 *
 *
 */
#include "../../../../include/core/cnxvector_registers.h"
#include "../../../../include/core/cnxvector.h"
#include "../../../../include/core/io_unit.h"
#include "../../../../include/c_simu/c_simulator.h"
#include "../../../../include/util/utils.h"
#include "../../../../include/util/timing.h"
#include "../../../../include/util/kernel_acc.h"

#include <iostream>
#include <iomanip>

using namespace std;

#define AES_KEY_EXPANSION_BNR   0
#define AES_ENCRYPTION_BNR      1
//#define PARALLEL_AES_ENCRYPTIONS 128
#define AES_TIMING_LOOPS 10000

#define Nb 4

/* 128 bit key */
#define Nk 4 /* key length in words */
#define Nr (Nk + 6) /* number of rounds */

UINT8 UINT8_TestKEY[] = {0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6, 0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c};
//UINT8 TestKEY[AES_KEY_SIZE_IN_BYTES] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};

UINT8 plaintext[4*Nb] = {0x32, 0x43, 0xf6, 0xa8, 0x88, 0x5a, 0x30, 0x8d,0x31, 0x31, 0x98, 0xa2, 0xe0, 0x37, 0x07, 0x34};
//UINT8 plaintext[4*Nb] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};

UINT8 cryptotext0[4*Nb] = {0x39, 0x25, 0x84, 0x1d, 0x02, 0xdc, 0x09, 0xfb,
                            0xdc, 0x11, 0x85, 0x97, 0x19, 0x6a, 0x0b, 0x32};

UINT8 cryptotext127[4*Nb] = {0x0f, 0x4e, 0xfe, 0x64, 0xf0, 0x6e, 0xb9, 0x95,
                            0x76, 0x9a, 0xf0, 0xc8, 0x0f, 0x99, 0x18, 0xf2};

//UINT8 cryptotext127[4*Nb] = {};

/*
This program is only for the 128-bit version, but the 192-bit and 256-bit key versions are similar.
AES is designed to work on bytes. However, each byte is interperted as a representation of the polynomial:
b7x7 + b6x6 + b5x5 + b4x4 + b3x3 + b2x2 + b1x + b0

xn = x^n;
addition = XOR
multiplication is done modulo x8 + x4 + x3 + x + 1;

*/


void printU128(UINT8 *data)
{
    for (int i=0; i< 128/8; i++)
    {
        for (int j=7; j>= 0; j--)
        {
            //cout << ((data[128/8 - i - 1] >> j) & 0x01);
            cout << ((data[i] >> j) & 0x01);
        }
        cout << endl;
    }
}

UINT8 AES_SBOX[16*16] =
{
    0x63,0x7c,0x77,0x7b,0xf2,0x6b,0x6f,0xc5,0x30,0x01,0x67,0x2b,0xfe,0xd7,0xab,0x76,
    0xca,0x82,0xc9,0x7d,0xfa,0x59,0x47,0xf0,0xad,0xd4,0xa2,0xaf,0x9c,0xa4,0x72,0xc0,
    0xb7,0xfd,0x93,0x26,0x36,0x3f,0xf7,0xcc,0x34,0xa5,0xe5,0xf1,0x71,0xd8,0x31,0x15,
    0x04,0xc7,0x23,0xc3,0x18,0x96,0x05,0x9a,0x07,0x12,0x80,0xe2,0xeb,0x27,0xb2,0x75,
    0x09,0x83,0x2c,0x1a,0x1b,0x6e,0x5a,0xa0,0x52,0x3b,0xd6,0xb3,0x29,0xe3,0x2f,0x84,
    0x53,0xd1,0x00,0xed,0x20,0xfc,0xb1,0x5b,0x6a,0xcb,0xbe,0x39,0x4a,0x4c,0x58,0xcf,
    0xd0,0xef,0xaa,0xfb,0x43,0x4d,0x33,0x85,0x45,0xf9,0x02,0x7f,0x50,0x3c,0x9f,0xa8,
    0x51,0xa3,0x40,0x8f,0x92,0x9d,0x38,0xf5,0xbc,0xb6,0xda,0x21,0x10,0xff,0xf3,0xd2,
    0xcd,0x0c,0x13,0xec,0x5f,0x97,0x44,0x17,0xc4,0xa7,0x7e,0x3d,0x64,0x5d,0x19,0x73,
    0x60,0x81,0x4f,0xdc,0x22,0x2a,0x90,0x88,0x46,0xee,0xb8,0x14,0xde,0x5e,0x0b,0xdb,
    0xe0,0x32,0x3a,0x0a,0x49,0x06,0x24,0x5c,0xc2,0xd3,0xac,0x62,0x91,0x95,0xe4,0x79,
    0xe7,0xc8,0x37,0x6d,0x8d,0xd5,0x4e,0xa9,0x6c,0x56,0xf4,0xea,0x65,0x7a,0xae,0x08,
    0xba,0x78,0x25,0x2e,0x1c,0xa6,0xb4,0xc6,0xe8,0xdd,0x74,0x1f,0x4b,0xbd,0x8b,0x8a,
    0x70,0x3e,0xb5,0x66,0x48,0x03,0xf6,0x0e,0x61,0x35,0x57,0xb9,0x86,0xc1,0x1d,0x9e,
    0xe1,0xf8,0x98,0x11,0x69,0xd9,0x8e,0x94,0x9b,0x1e,0x87,0xe9,0xce,0x55,0x28,0xdf,
    0x8c,0xa1,0x89,0x0d,0xbf,0xe6,0x42,0x68,0x41,0x99,0x2d,0x0f,0xb0,0x54,0xbb,0x16
};

void AES_CnxMixColumn(cnxvector r0, cnxvector r1, cnxvector r2, cnxvector r3,
                      cnxvector a0, cnxvector a1, cnxvector a2, cnxvector a3,
                      cnxvector b0, cnxvector b1, cnxvector b2, cnxvector b3,
                      cnxvector h, cnxvector manff, cnxvector man1b, cnxvector man128
                      )
{
    /* The array 'a' is simply a copy of the input array 'r'
     * The array 'b' is each element of the array 'a' multiplied by 2
     * in Rijndael's Galois field
     * a[n] ^ b[n] is element n multiplied by 3 in Rijndael's Galois field */

    a0 = r0;
    a1 = r1;
    a2 = r2;
    a3 = r3;

    /* h is 0xff if the high bit of r[c] is set, 0 otherwise */
    h = r0;
    h = ULT(h, man128);// ULT returns 1 for true and 0 for false
    h += manff;
    h = h & man1b;

    b0 = r0 << 1; /* we will xor by 0x1b in the next line */
    b0 ^= h;
    b0 &= manff;

    h = r1;
    h = ULT(h, man128);// ULT returns 1 for true and 0 for false
    h += manff;
    h = h & man1b;

    b1 = r1 << 1; /* we will xor by 0x1b in the next line */
    b1 ^= h;
    b1 &= manff;


    h = r2;
    h = ULT(h, man128);// ULT returns 1 for true and 0 for false
    h += manff;
    h = h & man1b;

    b2 = r2 << 1; /* we will xor by 0x1b in the next line */
    b2 ^= h;
    b2 &= manff;


    h = r3;
    h = ULT(h, man128);// ULT returns 1 for true and 0 for false
    h += manff;
    h = h & man1b;

    b3 = r3 << 1; /* we will xor by 0x1b in the next line */
    b3 ^= h;
    b3 &= manff;


    r0 = b0;
    r0 ^= a3;
    r0 ^= a2;
    r0 ^= b1;
    r0 ^= a1; // 2 * a0 + a3 + a2 + 3 * a1

    r1 = b1;
    r1 ^= a0;
    r1 ^= a3;
    r1 ^= b2;
    r1 ^= a2; // 2 * a1 + a0 + a3 + 3 * a2

    r2 = b2;
    r2 ^= a1;
    r2 ^= a0;
    r2 ^= b3;
    r2 ^= a3; // 2 * a2 + a1 + a0 + 3 * a3

    r3 = b3;
    r3 ^= a2;
    r3 ^= a1;
    r3 ^= b0;
    r3 ^= a0; // 2 * a3 + a2 + a1 + 3 * a0

}

void AES_CnxMixColumns()
{
    R25 = 0xff;
    R26 = 0x1b;
    R27 = 128;

    for (int i =0; i< 4; i++)
        AES_CnxMixColumn (R[4*i], R[4*i + 1], R[4*i + 2], R[4*i + 3],
                            R[16], R[17], R[18], R[19],
                            R[20], R[21], R[22], R[23],
                            R24,R25, R26, R27);
}

/* LocalStore contains:

0   ... 255     AES_SBOX[index]: Keep it at index 0. This is optimal, since we do not have instruction for accessing LS[Rx + offset]
                // If you modify it however, add extra steps (aka "+" with offset) in AES_CnxSubWord / AES_CnxSubBytes
256 ... 288     AES_KEYS (in BYTES)
288 ... 288 + Nb*(Nr+1): wi: = 288 + 4*44: wi (for 128 bit) AES_Plaintext

*/
#define AES_KEY_SIZE_IN_BYTES (128/8)
#define AES_KEY_SIZE_IN_WORDS   8

#define LOCAL_STORE_SBOX_OFFSET 0
#define AES_SBOX_SIZE 256

#define LOCAL_STORE_KEYS_OFFSET AES_SBOX_SIZE
#define LOCAL_STORE_WI_OFFSET (AES_SBOX_SIZE + AES_KEY_SIZE_IN_BYTES)

#define AES_DATABLOCK_SIZE_IN_BYTES (128/8)

#define LOCAL_STORE_PLAINTEXT_OFFSET(x) ((LOCAL_STORE_WI_OFFSET + 4*Nb*(Nr + 1)) + x*AES_DATABLOCK_SIZE_IN_BYTES)
#define LOCAL_STORE_CRYPTOTEXT_OFFSET(x) (LOCAL_STORE_PLAINTEXT_OFFSET(MAX_DATABLOCKS_IN_LOCALSTORE) \
                                            + x*AES_DATABLOCK_SIZE_IN_BYTES)

#define LOCAL_STORE_END(x) (LOCAL_STORE_CRYPTOTEXT_OFFSET(x) + AES_DATABLOCK_SIZE_IN_BYTES)

#define MAX_DATABLOCKS_IN_LOCALSTORE 15
#define DATABLOCK_SIZE (NUMBER_OF_MACHINES * AES_DATABLOCK_SIZE_IN_BYTES)

UINT_REGISTER_VAL CnxSbox[NUMBER_OF_MACHINES * AES_SBOX_SIZE];
void CreateSbox(UINT_REGISTER_VAL *CS)
{
    for (int i=0; i < AES_SBOX_SIZE; i++)
    for (int machine=0; machine < NUMBER_OF_MACHINES; machine++)
    CS[i * NUMBER_OF_MACHINES + machine] = AES_SBOX[i];
}

io_unit IOU_Sbox;
void CnxPreprareTransferSbox(UINT_REGISTER_VAL *CS)
{
    IOU_Sbox.preWritecnxvectors(LOCAL_STORE_SBOX_OFFSET,CS, AES_SBOX_SIZE);
}

int AES_CnxTransferSbox()
{
    if (PASS != IO_WRITE_NOW(&IOU_Sbox)){printf("Writing to IO pipe, FAILED !"); return FAIL;}
}

UINT_REGISTER_VAL CnxKeys[NUMBER_OF_MACHINES * AES_KEY_SIZE_IN_BYTES];
io_unit IOU_Keys;
void CreateKeys(UINT_REGISTER_VAL *CK)
{
    for (int i=0; i < AES_KEY_SIZE_IN_BYTES; i++)
        for (int machine=0; machine < NUMBER_OF_MACHINES; machine++)
        {
            if (i == (AES_KEY_SIZE_IN_BYTES - 1))
               CK[i * NUMBER_OF_MACHINES + machine] = UINT8_TestKEY[i] + machine; //make a small difference in the key across machines
            else
                CK[i * NUMBER_OF_MACHINES + machine] = UINT8_TestKEY[i];

            //cout<<"!"<<CK[i * NUMBER_OF_MACHINES]<<"!"<<endl;
        }
}

void CnxPreprareTransferKeys(UINT_REGISTER_VAL *CK)
{
    IOU_Keys.preWritecnxvectors(LOCAL_STORE_KEYS_OFFSET,CK,AES_KEY_SIZE_IN_BYTES);
}

int AES_CnxTransferKeys()
{
    if (PASS != IO_WRITE_NOW(&IOU_Keys)){printf("Writing to IO pipe, FAILED !"); return FAIL;}
}


UINT_REGISTER_VAL CnxDataInput[MAX_DATABLOCKS_IN_LOCALSTORE * DATABLOCK_SIZE];
void CreateInputDataBlocks(UINT_REGISTER_VAL *CDI, int datablocks)
{
    for (int db = 0; db < datablocks; db ++)
        for (int i=0; i < AES_DATABLOCK_SIZE_IN_BYTES; i++)
            for (int machine=0; machine < NUMBER_OF_MACHINES; machine++)
            {
                if (i == (AES_DATABLOCK_SIZE_IN_BYTES - 1))
                    //make a small difference in the key across machines
                    CnxDataInput[db*DATABLOCK_SIZE + (i * NUMBER_OF_MACHINES + machine)] = plaintext[i] + db + machine;
                else
                    CnxDataInput[db*DATABLOCK_SIZE + (i * NUMBER_OF_MACHINES + machine)] = plaintext[i];
            }
}

io_unit IOU_CnxDataInput;
void CnxPreprareTransferInputDataBlocks(UINT_REGISTER_VAL *CDI, int datablocks)
{
    IOU_CnxDataInput.preWritecnxvectors(LOCAL_STORE_PLAINTEXT_OFFSET(0),CDI, datablocks*AES_DATABLOCK_SIZE_IN_BYTES);
}
int AES_CnxTransferInputDataBlocks()
{
    if (PASS != IO_WRITE_NOW(&IOU_CnxDataInput)){printf("Writing to IO pipe, FAILED !"); return FAIL;}
    return PASS;
}

io_unit IOU_CnxDataOutput;
void CnxPreprareTransferOutputDataBlocks(int datablocks)
{
    IOU_CnxDataOutput.preReadcnxvectors(LOCAL_STORE_CRYPTOTEXT_OFFSET(0), datablocks*AES_DATABLOCK_SIZE_IN_BYTES);
}

int AES_CnxTransferOutputDataBlocks()
{
    if (PASS != IO_READ_NOW(&IOU_CnxDataOutput)){printf("Reading from IO pipe, FAILED !"); return FAIL;}
}

#define USING4REGS(a,b,c,d) a,b,c,d
/**
    SubWord the UINT32 (Rx concatenated with Ry), using Rm* registers.
*/
void AES_CnxSubWord(cnxvector Rx, cnxvector Ry, cnxvector Rz, cnxvector Rt)
{
    Rx = LS[Rx];
    Ry = LS[Ry];
    Rz = LS[Rz];
    Rt = LS[Rt];
}

void AES_Copyw0_w3()
{
    // 4 is UINT32/ UINT8
    for (int i= 0; i < 4*Nk; i+= Nk)
    {
        int j = i & 0x3;
        R[j] = LS[LOCAL_STORE_KEYS_OFFSET+i];
        R[j+1] = LS[LOCAL_STORE_KEYS_OFFSET+i+1];
        R[j+2] = LS[LOCAL_STORE_KEYS_OFFSET+i+2];
        R[j+3] = LS[LOCAL_STORE_KEYS_OFFSET+i+3];

        LS[LOCAL_STORE_WI_OFFSET + i]   = R[j];
        LS[LOCAL_STORE_WI_OFFSET + i+1] = R[j+1];
        LS[LOCAL_STORE_WI_OFFSET + i+2] = R[j+2];
        LS[LOCAL_STORE_WI_OFFSET + i+3] = R[j+3];
    }
}

UINT8 AES_CnxRcon[] = {0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20 , 0x40, 0x80, 0x1b, 0x36};
void AES_CnxXorRcon(cnxvector Rx, cnxvector Ry, cnxvector Rz, cnxvector Rt, int val, cnxvector Rman)
{
    Rman = val;
    Ry = Ry ^ Rman;
}

void AES_CnxGetWi(cnxvector Rx, cnxvector Ry, cnxvector Rz, cnxvector Rt, int windex)
{
    Rx = LS[LOCAL_STORE_WI_OFFSET + 4*windex    ];
    Ry = LS[LOCAL_STORE_WI_OFFSET + 4*windex + 1];
    Rz = LS[LOCAL_STORE_WI_OFFSET + 4*windex + 2];
    Rt = LS[LOCAL_STORE_WI_OFFSET + 4*windex + 3];
}

void AES_CnxSetWi(cnxvector Rx, cnxvector Ry, cnxvector Rz, cnxvector Rt, int windex)
{
    LS[LOCAL_STORE_WI_OFFSET + 4*windex    ] = Rx;
    LS[LOCAL_STORE_WI_OFFSET + 4*windex + 1] = Ry;
    LS[LOCAL_STORE_WI_OFFSET + 4*windex + 2] = Rz;
    LS[LOCAL_STORE_WI_OFFSET + 4*windex + 3] = Rt;
}

void AES_CnxXorWiWimNk(cnxvector Rx, cnxvector Ry, cnxvector Rz, cnxvector Rt,
                    cnxvector Ra, cnxvector Rb, cnxvector Rc, cnxvector Rd)
{
    Rx = Rx ^ Ra;
    Ry = Ry ^ Rb;
    Rz = Rz ^ Rc;
    Rt = Rt ^ Rd;
}

int AES_CnxKeyExpansion()
{
    UINT32 temp;
    UINT8 i = Nk;

    /* copy w0...w3 from localstore (key) to localstore (wi)
        R0...R3 will eventually have w3 in them (first w[i-1])
    */
    AES_Copyw0_w3();
    while (i < Nb * (Nr+1))
    {
        if (i != Nk) //unless first iteration, when we already have R0...R3 filled with w3
        {
            AES_CnxGetWi(R0,R1,R2,R3, i - 1); // load w[i-1]
        }

        //temp = w[i-1];temp is R0...R3
        if ((i % Nk) == 0)
        {
            //UINT32 rw = RotWord(temp); RotWord will be delayed: NB that Rcon is also RotWord-ed !
            AES_CnxSubWord(R0,R1,R2,R3);// UINT32 sw = SubWord(rw);
            AES_CnxXorRcon(R0,R1,R2,R3, AES_CnxRcon[i/Nk], R4);// temp = sw xor Rcon[i/Nk];
        }
        else if ((Nk > 6) && (i % Nk == 4))
            AES_CnxSubWord(R0,R1,R2,R3);// temp = SubWord(temp);


        AES_CnxGetWi(R4,R5,R6,R7, i - Nk); // load w[i-Nk]


        if ((i % Nk) == 0)
        {
            AES_CnxXorWiWimNk(R1,R2,R3,R0, R4,R5,R6,R7); // w[i-Nk] ^ temp; remember RotWord pending
            AES_CnxSetWi(R1,R2,R3,R0, i); //w[i] = w[i-Nk] ^ temp; Also make the RotWord here.
        }
        else
        {
            AES_CnxXorWiWimNk(R0,R1,R2,R3,R4,R5,R6,R7); // w[i-Nk] ^ temp; no RotWord pending
            AES_CnxSetWi(R0,R1,R2,R3, i); //w[i] = w[i-Nk] ^ temp;
        }
        i++;
    }
    return PASS;
}

void AES_LoadPlainText(int DataPair)
{
    for (int i= 0; i < 4*Nb; i++)
        R[i] = LS[LOCAL_STORE_PLAINTEXT_OFFSET(DataPair) + i];
}

void AES_StoreCryptoText(int DataPair)
{
    for (int i= 0; i < 4*Nb; i++)
        LS[LOCAL_STORE_CRYPTOTEXT_OFFSET(DataPair) + i] = R[i];
}

void AES_CnxAddRoundKey(int startIndex, int lastIndex)
{
    //load RoundKey to R16...R31
    for (int i= 0; i < 4*Nb; i++)
        R[16 + i] = LS[LOCAL_STORE_WI_OFFSET + 4*startIndex + i];

    //add it to state
    for (int row=0; row< 4; row++)
      for (int col=0; col< Nb; col++)
         R[row*Nb + col] = R[row*Nb + col] ^ R[16 + row*Nb + col];
}

void AES_CnxSubBytes()
{
    for (int i= 0; i < 4*Nb; i++)
        R[i] = LS[R[i]];
}

void AES_CnxShiftRows()
{
    //row = 1;
    {
       R16 = R[1];
       R[1] = R[5];
       R[5] = R[9];
       R[9] = R[13];
       R[13] = R16;
    }

    //row = 2;
    {
       R16 = R[2];
       R[2] = R[10];
       R[10] = R16;

       R16 = R[6];
       R[6] = R[14];
       R[14] = R16;
    }

    //row = 3;
    {
       R16 = R[15];
       R[15] = R[11];
       R[11] = R[7];
       R[7]  = R[3];
       R[3]  = R16;
    }
}

void AES_CnxEncryption(int datablock)
{
    /* Load first 128 datablocks from localstore */
    AES_LoadPlainText(datablock);// load plaintext starting with R0: R0 ... R15 has now the plaintext;
    AES_CnxAddRoundKey(0,Nb-1);

    for (int round = 1; round <= Nr-1; round++)
    {
        AES_CnxSubBytes(); // See Sec. 5.1.1
        AES_CnxShiftRows(); // See Sec. 5.1.2
        AES_CnxMixColumns(); // See Sec. 5.1.3
        AES_CnxAddRoundKey(round*Nb,(round+1)*Nb-1);
    }

    AES_CnxSubBytes(); // See Sec. 5.1.1
    AES_CnxShiftRows(); // See Sec. 5.1.2
    AES_CnxAddRoundKey(Nr*Nb,(Nr+1)*Nb-1);

    AES_StoreCryptoText(datablock);
}

void print_AES_Wi(int index)
{
    for (int offset = index*4; offset < index*4 + 4; offset++)
    c_simulator::printLS(LOCAL_STORE_WI_OFFSET + offset,0);
}

void print_AES_Plaintext(int index, int DataPair)
{
    for (int offset = index; offset < index + 16; offset++)
    c_simulator::printLS(LOCAL_STORE_PLAINTEXT_OFFSET(DataPair) + offset,0);
}

void CreateKeyExpansionKernel(int bnr)
{
    BEGIN_BATCH(bnr);
        AES_CnxKeyExpansion();
    END_BATCH(bnr);
}

void CreateAesEncryptionKernel(int bnr, int datablocks)
{
    BEGIN_BATCH(bnr);
    for (int db=0; db < datablocks; db++)
        AES_CnxEncryption(db);
    END_BATCH(bnr);
}

int AES_ConnexSEncryption()
{
    cout<<"AES encryption test: encryption of  "<< NUMBER_OF_MACHINES
            <<" parallel blocks of input data, "<< AES_TIMING_LOOPS<<" loops "<<endl;

    int TimeStart = GetMilliCount();
    CreateKeys(CnxKeys);
    CreateSbox(CnxSbox);
    CreateInputDataBlocks(CnxDataInput, MAX_DATABLOCKS_IN_LOCALSTORE);
    cout<<" Time for creation of data, keys, sbox "<< GetMilliSpan(TimeStart)<<" ms"<<endl;

    TimeStart = GetMilliCount();
    CnxPreprareTransferKeys(CnxKeys);
    CnxPreprareTransferSbox(CnxSbox);
    CnxPreprareTransferInputDataBlocks(CnxDataInput, MAX_DATABLOCKS_IN_LOCALSTORE);
    CnxPreprareTransferOutputDataBlocks(MAX_DATABLOCKS_IN_LOCALSTORE);
    cout<<" Time for preparing transfers via IO "<< GetMilliSpan(TimeStart)<<" ms"<<endl;

    TimeStart = GetMilliCount();
    CreateKeyExpansionKernel(AES_KEY_EXPANSION_BNR);
    CreateAesEncryptionKernel(AES_ENCRYPTION_BNR, MAX_DATABLOCKS_IN_LOCALSTORE);
    cout<<" Time for creating batches "<< GetMilliSpan(TimeStart)<<" ms"<<endl;

    TimeStart = GetMilliCount();
    AES_CnxTransferSbox();
    AES_CnxTransferKeys();
    cout<<" Time for transfering Sbox and Keys "<< GetMilliSpan(TimeStart)<<" ms"<<endl;

    int TimeIO = 0;
    int TimeRunKernel = 0;
    for (int i=0; i < AES_TIMING_LOOPS; i++)
    {
        TimeStart = GetMilliCount();
            AES_CnxTransferInputDataBlocks();
        TimeIO += GetMilliSpan(TimeStart);

        TimeStart = GetMilliCount();
            EXECUTE_BATCH(AES_KEY_EXPANSION_BNR);
            EXECUTE_BATCH(AES_ENCRYPTION_BNR);
        TimeRunKernel += GetMilliSpan(TimeStart);

        TimeStart = GetMilliCount();
            AES_CnxTransferOutputDataBlocks();
        TimeIO += GetMilliSpan(TimeStart);
    }

    cout<<" Time for transfering input/output data via IO "<< TimeIO <<" ms"<<endl;
    cout<<" Time for running kernels "<< TimeRunKernel <<"ms"<<endl;
    cout<< (((float)AES_TIMING_LOOPS) / (TimeRunKernel + TimeIO)) <<" Kblocks/s per machine "<<endl;
    cout<< NUMBER_OF_MACHINES*(((float)AES_TIMING_LOOPS) / (TimeRunKernel + TimeIO)) <<" Kblocks/s per ConnexS "<<endl;

    //NUMBER_OF_MACHINES
    UINT16 *CryptoContent = (UINT16*)((IOU_CnxDataOutput.getIO_UNIT_CORE())->Content);
    CryptoContent = (UINT16*)((IOU_CnxDataOutput.getIO_UNIT_CORE())->Content);
    for (int i=0; i< AES_DATABLOCK_SIZE_IN_BYTES; i++)
        if (cryptotext0[i] != CryptoContent[NUMBER_OF_MACHINES * i])
        {
            cout<<"FAIL"<<endl;
            cout<< "Failed on cryptotext 0 !"<<CryptoContent[NUMBER_OF_MACHINES * i]<<endl;
            return FAIL;
        }

    CryptoContent = (UINT16*)((IOU_CnxDataOutput.getIO_UNIT_CORE())->Content);
    for (int i=0; i< AES_DATABLOCK_SIZE_IN_BYTES; i++)
        if (cryptotext127[i] != CryptoContent[127 + NUMBER_OF_MACHINES * i])
        {
            cout<<"FAIL"<<endl;
            cout<< "Failed on cryptotext 127 !"<<CryptoContent[127 + NUMBER_OF_MACHINES * i]<<endl;
            return FAIL;
        }

    //print_AES_Plaintext(0);
    //for (int r=0;r<16;r++)
      //c_simulator::printREG(r,0);

    //c_simulator::printLS(r+LOCAL_STORE_CRYPTOTEXT_OFFSET,0);

    //DEASM_BATCH(AES_ENCRYPTION_BNR);
    cout<<"PASS";
}

int test_AES_All()
{
    cout<<" LOCAL_STORE_END = "<<LOCAL_STORE_END(17)<<endl;
    cout<<" LOCAL_STORE_PLAINTEXT_OFFSET(MAX) = "<<LOCAL_STORE_PLAINTEXT_OFFSET(MAX_DATABLOCKS_IN_LOCALSTORE)<<endl;
    cout<<" LOCAL_STORE_CRYPTOTEXT_OFFSET(0) = "<<LOCAL_STORE_CRYPTOTEXT_OFFSET(0)<<endl;
    cout<<" LOCAL_STORE_CRYPTOTEXT_OFFSET(MAX) = "<<LOCAL_STORE_CRYPTOTEXT_OFFSET(MAX_DATABLOCKS_IN_LOCALSTORE)<<endl;
    int Start;
    //Start = GetMilliCount();
    //cout<<"Batches were created in " << GetMilliSpan(Start)<< " ms"<<endl;
    AES_ConnexSEncryption();
    //connexFindMatchesPass1(MODE_CREATE_BATCHES, BASIC_MATCHING_BNR, &SiftDescriptors1, &SiftDescriptors2, &SM_ConnexArm);
	return 0;
}




