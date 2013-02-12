
/*
 *
 * File: aes_tests.cpp
 *
 * AES tests.
 *
 *
 */
#include "../../../../include/core/io_unit.h"
#include "../../../../include/c_simu/c_simulator.h"
#include "../../../../include/util/utils.h"
#include "../../../../include/util/timing.h"
#include "../../../../include/util/kernel_acc.h"

#include <iostream>
#include <iomanip>

using namespace std;
#define Nb 4

/* 128 bit key */
#define Nk 4 /* key length in words */
#define Nr (Nk + 6) /* number of rounds */
#define WI_SIZE_IN_DWORDS (Nk * (Nr + 1))
#define AES_DATABLOCK_SIZE_IN_DWORDS (128/32)
UINT8 UINT8_TestKEY[] = {0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6, 0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c};
UINT32 UINT32_TestKEY[] = {0x2b7e1516, 0x28aed2a6, 0xabf71588, 0x09cf4f3c};
//UINT8 TestKEY[AES_KEY_SIZE_IN_BYTES] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};

//UINT8 UINT8_plaintext[4*Nb] = {0x32, 0x43, 0xf6, 0xa8, 0x88, 0x5a, 0x30, 0x8d,0x31, 0x31, 0x98, 0xa2, 0xe0, 0x37, 0x07, 0x34};
UINT32 UINT32_plaintext[Nb] = {0x3243f6a8, 0x885a308d,0x313198a2, 0xe0370734};
//UINT8 plaintext[4*Nb] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};

UINT32 UINT32_cryptotext[Nb] = {0,0,0,0};
UINT32 UINT32_expected_cryptotext[Nb] = {0x3925841d,0x02dc09fb,0xdc118597,0x196a0b32};
//UINT8 UINT8_cryptotext[4*Nb] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

UINT8 cryptotext0[4*Nb] = {0x39, 0x25, 0x84, 0x1d, 0x02, 0xdc, 0x09, 0xfb,
                            0xdc, 0x11, 0x85, 0x97, 0x19, 0x6a, 0x0b, 0x32};

UINT8 cryptotext127[4*Nb] = {0x0f, 0x4e, 0xfe, 0x64, 0xf0, 0x6e, 0xb9, 0x95,
                            0x76, 0x9a, 0xf0, 0xc8, 0x0f, 0x99, 0x18, 0xf2};


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


void AES_MixColumn(UINT8 *r)
{
    unsigned char a[4];
    unsigned char b[4];
    unsigned char c;
    unsigned char h;
    /* The array 'a' is simply a copy of the input array 'r'
     * The array 'b' is each element of the array 'a' multiplied by 2
     * in Rijndael's Galois field
     * a[n] ^ b[n] is element n multiplied by 3 in Rijndael's Galois field */

    a[0] = r[3];a[1] = r[2];a[2] = r[1];a[3] = r[0];

    /* h is 0xff if the high bit of r[c] is set, 0 otherwise */
    h = (unsigned char)((signed char)r[3] >> 7); /* arithmetic right shift, thus shifting in either zeros or ones */
    b[0] = r[3] << 1; /* implicitly removes high bit because b[c] is an 8-bit char, so we xor by 0x1b and not 0x11b in the next line */
    b[0] ^= 0x1B & h; /* Rijndael's Galois field */
    //cout<<endl<<(int)b[0]<<" ENDDD"<<endl;


   /* h is 0xff if the high bit of r[c] is set, 0 otherwise */
    h = (unsigned char)((signed char)r[2] >> 7); /* arithmetic right shift, thus shifting in either zeros or ones */
    b[1] = r[2] << 1; /* implicitly removes high bit because b[c] is an 8-bit char, so we xor by 0x1b and not 0x11b in the next line */
    b[1] ^= 0x1B & h; /* Rijndael's Galois field */
    //cout<<endl<<(int)b[1]<<" ENDDD"<<endl;

   /* h is 0xff if the high bit of r[c] is set, 0 otherwise */
    h = (unsigned char)((signed char)r[1] >> 7); /* arithmetic right shift, thus shifting in either zeros or ones */
    b[2] = r[1] << 1; /* implicitly removes high bit because b[c] is an 8-bit char, so we xor by 0x1b and not 0x11b in the next line */
    b[2] ^= 0x1B & h; /* Rijndael's Galois field */
    //cout<<endl<<(int)b[2]<<" ENDDD"<<endl;

   /* h is 0xff if the high bit of r[c] is set, 0 otherwise */
    h = (unsigned char)((signed char)r[0] >> 7); /* arithmetic right shift, thus shifting in either zeros or ones */
    b[3] = r[0] << 1; /* implicitly removes high bit because b[c] is an 8-bit char, so we xor by 0x1b and not 0x11b in the next line */
    b[3] ^= 0x1B & h; /* Rijndael's Galois field */
    //cout<<endl<<(int)b[3]<<" ENDDD"<<endl;
//return;

    r[3] = b[0] ^ a[3] ^ a[2] ^ b[1] ^ a[1]; /* 2 * a0 + a3 + a2 + 3 * a1 */
    r[2] = b[1] ^ a[0] ^ a[3] ^ b[2] ^ a[2]; /* 2 * a1 + a0 + a3 + 3 * a2 */
    r[1] = b[2] ^ a[1] ^ a[0] ^ b[3] ^ a[3]; /* 2 * a2 + a1 + a0 + 3 * a3 */
    r[0] = b[3] ^ a[2] ^ a[1] ^ b[0] ^ a[0]; /* 2 * a3 + a2 + a1 + 3 * a0 */

    return;
}

#define AES_TIMING_LOOPS (666*15* 128)

#define AES_KEY_SIZE_IN_BYTES (128/8)
#define AES_KEY_SIZE_IN_WORDS   8

#define AES_SBOX_SIZE 256
#define AES_DATABLOCK_SIZE_IN_BYTES (128/8)
#define DATABLOCK_SIZE (NUMBER_OF_MACHINES * AES_DATABLOCK_SIZE_IN_BYTES)
/*
UINT_REGISTER_VAL Keys[NUMBER_OF_MACHINES * AES_KEY_SIZE_IN_BYTES];
void CreateKeys(UINT_REGISTER_VAL *CK)
{
    for (int i=0; i < AES_KEY_SIZE_IN_BYTES; i++)
        for (int machine=0; machine < NUMBER_OF_MACHINES; machine++)
        {
            if (i == (AES_KEY_SIZE_IN_BYTES - 1))
               CK[i * NUMBER_OF_MACHINES + machine] = UINT8_TestKEY[i] + machine; //make a small difference in the key across machines
            else
                CK[i * NUMBER_OF_MACHINES + machine] = UINT8_TestKEY[i];
        }
}
*/
union _Wi
{
    UINT8 Bytes[4];
    UINT32 DoubleWord;
};

_Wi Wis[Nb * (Nr+1)];

/**
    SubWord the UINT32
*/
void AES_SubWord(_Wi *wi)
{
    wi->Bytes[0] = AES_SBOX[wi->Bytes[0]];
    wi->Bytes[1] = AES_SBOX[wi->Bytes[1]];
    wi->Bytes[2] = AES_SBOX[wi->Bytes[2]];
    wi->Bytes[3] = AES_SBOX[wi->Bytes[3]];
}

union _State
{
    UINT32 dwColumns[4];
    UINT8 bCells[16];
};

void AES_SubBytes(_State *s)
{
    for (int i=0; i< Nb * 4; i++)
        s->bCells[i] = AES_SBOX[s->bCells[i]];
}

void AES_MixColumns(_State *s)
{
    for (int i =0; i< 4; i++)
        AES_MixColumn ((UINT8*)&s->dwColumns[i]);
}

UINT8 AES_Rcon[] = {0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20 , 0x40, 0x80, 0x1b, 0x36};
int AES_KeyExpansion(_Wi *Wi, UINT32 *Key)
{
    UINT32 temp;
    UINT8 i;
    for (i=0; i < Nk; i++)
        Wi[i].DoubleWord = Key[i];

    while (i < Nb * (Nr+1))
    {
        Wi[i].DoubleWord = Wi[i-1].DoubleWord;
        if ((i % Nk) == 0)
        {
            //UINT32 rw = RotWord(temp); RotWord will be delayed: NB that Rcon is also RotWord-ed !
            //cout << "Wi[" <<(UINT32)i << "]= " << hex << (UINT32)Wi[i].DoubleWord <<dec <<endl;
            //cout << "Wi[" <<(UINT32)i << "]= " << hex << (UINT32)Wi[i].DoubleWord <<dec <<endl;
            AES_SubWord(&Wi[i]);// UINT32 sw = SubWord(rw);
            //cout << "Wi[" <<(UINT32)i << "]= " << hex << (UINT32)Wi[i].DoubleWord <<dec <<endl;
            //AES_XorRcon(QState, AES_Rcon[i/Nk]);// temp = sw xor Rcon[i/Nk];
            //cout << "Wi[" <<(UINT32)i << "]= " << hex << (UINT32)Wi[i].DoubleWord <<dec <<endl;
            //cout << "Wi[" <<(UINT32)i << "]= " << hex << (UINT32)Wi[i].DoubleWord <<dec <<endl;
            Wi[i].Bytes[2] ^= AES_Rcon[i/Nk];
            //cout << "Wi[" <<(UINT32)i << "]= " << hex << (UINT32)Wi[i].DoubleWord <<dec <<endl;

            //cout << "Wi[" <<(UINT32)i << "]= " << hex << (UINT32)Wi[i].DoubleWord <<dec <<endl;
        }
        else if ((Nk > 6) && (i % Nk == 4))
            AES_SubWord(&Wi[i]);// temp = SubWord(temp);

        //AES_GetWi(R4,R5,R6,R7, i - Nk); // load w[i-Nk]
        if ((i % Nk) == 0)
        {

            //AES_XorWiWimNk(R1,R2,R3,R0, R4,R5,R6,R7); // w[i-Nk] ^ temp; remember RotWord pending
            //AES_SetWi(R1,R2,R3,R0, i); //w[i] = w[i-Nk] ^ temp; Also make the RotWord here.
            //Wi[i].DoubleWord ^= Wi[i-Nk].DoubleWord;
            //cout << "Wi[" <<(UINT32)i << "]= " << hex << (UINT32)Wi[i].DoubleWord <<dec <<endl;
            //cout << "Wi[" <<(UINT32)i-Nk << "]= " << hex << (UINT32)Wi[i-Nk].DoubleWord <<dec <<endl;
            Wi[i].Bytes[0] ^= Wi[i-Nk].Bytes[1];
            Wi[i].Bytes[1] ^= Wi[i-Nk].Bytes[2];
            Wi[i].Bytes[2] ^= Wi[i-Nk].Bytes[3];
            Wi[i].Bytes[3] ^= Wi[i-Nk].Bytes[0];
            Wi[i].DoubleWord = (Wi[i].DoubleWord >> 24) | (Wi[i].DoubleWord << 8);
            //cout << "Wi[" <<(UINT32)i << "]= " << hex << (UINT32)Wi[i].DoubleWord <<dec <<endl;
        }
        else
        {
            //AES_XorWiWimNk(R0,R1,R2,R3,R4,R5,R6,R7); // w[i-Nk] ^ temp; no RotWord pending
            //AES_SetWi(R0,R1,R2,R3, i); //w[i] = w[i-Nk] ^ temp;
            //cout << "Wi[" <<(UINT32)i << "]= " << hex << (UINT32)Wi[i].DoubleWord <<dec <<endl;
            Wi[i].DoubleWord ^= Wi[i-Nk].DoubleWord;
            //cout << "Wi[" <<(UINT32)i-Nk << "]= " << hex << (UINT32)Wi[i-Nk].DoubleWord <<dec <<endl;
        }
        i++;
    }
    return PASS;
}

void AES_AddRoundKey(_State* State, int pos)
{
    //add it to state
    //for (int row=0; row< 4; row++)
      for (int col=0; col< Nb; col++)
      {
         //cout<<hex<<State->dwColumns[col]<<" "<<Wis[pos + col].DoubleWord<<dec<<endl;
         State->dwColumns[col] ^= Wis[pos + col].DoubleWord;
      }

}

void AES_ShiftRows(_State *s)
{
    UINT8 man;

//    for (int i=0; i< 16;i++)
//        cout<<hex<<(UINT16)s->bCells[i] <<" ";
//    cout<<dec<<endl;
    //row = 1;
    {
       man = s->bCells[2];
       s->bCells[2] = s->bCells[6];
       s->bCells[6] = s->bCells[10];
       s->bCells[10] = s->bCells[14];
       s->bCells[14]  = man;
    }

    //row = 2;
    {
       man = s->bCells[1];
       s->bCells[1] = s->bCells[9];
       s->bCells[9] = man;

       man = s->bCells[5];
       s->bCells[5] = s->bCells[13];
       s->bCells[13] = man;
    }

    //row = 3;
    {
       man = s->bCells[0];
       s->bCells[0] = s->bCells[12];
       s->bCells[12] = s->bCells[8];
       s->bCells[8] = s->bCells[4];
       s->bCells[4] = man;
    }
}

void AES_LoadPlainText(_State *s)
{
    s->dwColumns[0] = UINT32_plaintext[0];
    s->dwColumns[1] = UINT32_plaintext[1];
    s->dwColumns[2] = UINT32_plaintext[2];
    s->dwColumns[3] = UINT32_plaintext[3];
}

void AES_StoreCryptoText(_State *s)
{
    UINT32_cryptotext[0] = s->dwColumns[0];
    UINT32_cryptotext[1] = s->dwColumns[1];
    UINT32_cryptotext[2] = s->dwColumns[2];
    UINT32_cryptotext[3] = s->dwColumns[3];
}

void print_AES_Plaintext()
{
    UINT32 *PlaintextContent = UINT32_plaintext;
    for (int i=0; i< AES_DATABLOCK_SIZE_IN_DWORDS; i++)
      cout<<hex<<PlaintextContent[i]<<" ";
    cout<<dec<<endl;
}

void print_AES_Cryptotext()
{
    UINT32 *CryptotextContent = UINT32_cryptotext;
    for (int i=0; i< AES_DATABLOCK_SIZE_IN_DWORDS; i++)
        cout<<hex<<CryptotextContent[i]<<" ";
    cout<<dec<<endl;
}

void print_AES_Wi()
{
    for (int i=0; i< WI_SIZE_IN_DWORDS; i++)
        cout<<hex<<Wis[i].DoubleWord<<" "<<dec<<endl;
}

void print_AES_State(_State *state)
{
    for (int i=0; i< Nb; i++)
        cout<<hex<<state->dwColumns[i]<<" "<<dec<<endl;
}

void AES_Block_Encryption()
{
    _State State;
    AES_LoadPlainText(&State);
    AES_AddRoundKey(&State,0);
    for (int round = 1; round <= Nr-1; round++)
    {
        AES_SubBytes(&State); // See Sec. 5.1.1
        AES_ShiftRows(&State); // See Sec. 5.1.2
        AES_MixColumns(&State); // See Sec. 5.1.3
        //print_AES_State(&State);return;
        AES_AddRoundKey(&State, round*Nb);
    }

    AES_SubBytes(&State); // See Sec. 5.1.1
    AES_ShiftRows(&State); // See Sec. 5.1.2
    AES_AddRoundKey(&State,Nr*Nb);
    AES_StoreCryptoText(&State);
}

int AES_Encryption()
{
    int TimeStart;
    cout<<"AES CPU encryption test: encryption of  "<< AES_TIMING_LOOPS<<" datablocks " <<endl;


    //print_AES_Plaintext();
    //print_AES_Cryptotext();

    int TimeRunEncryption = 0;
    int TimeRunKeyExpansion = 0;
    for (int i=0; i < AES_TIMING_LOOPS; i++)
    {
        TimeStart = GetMilliCount();
            AES_KeyExpansion(Wis,UINT32_TestKEY);
        TimeRunKeyExpansion += GetMilliSpan(TimeStart);

        TimeStart = GetMilliCount();
            AES_Block_Encryption();
        TimeRunEncryption += GetMilliSpan(TimeStart);

    }

    cout<<" Time for running key expansion "<< TimeRunKeyExpansion <<"ms"<<endl;
    cout<<" Time for running encryption "<< TimeRunEncryption <<"ms"<<endl;
    cout<< (float)AES_TIMING_LOOPS / TimeRunEncryption <<" Kblocks/s (without key expansion)"<<endl;
    cout<< (float)AES_TIMING_LOOPS / (TimeRunEncryption + TimeRunKeyExpansion) <<" Kblocks/s (with key expansion)"<<endl;

    for (int i=0; i < AES_DATABLOCK_SIZE_IN_DWORDS; i++)
            if (UINT32_expected_cryptotext[i] != UINT32_cryptotext[i]) {cout<<" FAIL"<<endl;return FAIL;}
    cout<<" PASS"<<endl;
    return PASS;
}


int test_AES_CPU_All()
{
//    cout<<" LOCAL_STORE_END = "<<LOCAL_STORE_END(17)<<endl;
//    cout<<" LOCAL_STORE_PLAINTEXT_OFFSET(MAX) = "<<LOCAL_STORE_PLAINTEXT_OFFSET(MAX_DATABLOCKS_IN_LOCALSTORE)<<endl;
//    cout<<" LOCAL_STORE_CRYPTOTEXT_OFFSET(0) = "<<LOCAL_STORE_CRYPTOTEXT_OFFSET(0)<<endl;
//    cout<<" LOCAL_STORE_CRYPTOTEXT_OFFSET(MAX) = "<<LOCAL_STORE_CRYPTOTEXT_OFFSET(MAX_DATABLOCKS_IN_LOCALSTORE)<<endl;
    //int Start;
    //Start = GetMilliCount();
    //cout<<"Batches were created in " << GetMilliSpan(Start)<< " ms"<<endl;
    AES_Encryption();
    //connexFindMatchesPass1(MODE_CREATE_BATCHES, BASIC_MATCHING_BNR, &SiftDescriptors1, &SiftDescriptors2, &SM_ConnexArm);
	return 0;
}




