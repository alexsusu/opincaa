
#include "../../../../include/test/crypto/bsDES/utils.h"
#include "../../../../include/test/crypto/bsDES/testdata.h"
#include "../../../../include/test/crypto/bsDES/algo.h"

/*
 * Unroll the bits contained in the plaintext, ciphertext, and key values.
 */
static UINTX p[64];
static UINTX c[64];
static UINTX k[56];


#define CLEAN_4P(ii)\
    p[ii] = 0;p[(ii)+1] = 0;p[(ii)+2] = 0;p[(ii)+3] = 0;

#define CLEAN_16P(ii)\
    CLEAN_4P((ii));CLEAN_4P((ii)+4);CLEAN_4P((ii)+8);CLEAN_4P((ii)+12)

#define CLEAN_ALLP\
    CLEAN_16P(0);CLEAN_16P(16);CLEAN_16P(32);CLEAN_16P(48);


    #define FOR_I(i)\
       if ((plaintext_p[(j << 3) + ((i) >> 3)] & (128 >> ((i) & 0x07))) != 0) p[63 - (i)] |= (1 << j);

    #define FOR_4I(i)\
       FOR_I(i);FOR_I(i+1);FOR_I(i+2);FOR_I(i+3);

    #define FOR_16I(i)\
        FOR_4I(i);FOR_4I(i+4);FOR_4I(i+8);FOR_4I(i+12);

    #define FOR_64I(i)\
        FOR_16I(i);FOR_16I(i+16);FOR_16I(i+32);FOR_16I(i+48);

static void unroll_bits(UINT8* plaintext_p,UINT8* cryptotext_p,UINT8* key_p)
{
	int	    i,j;
	UINT32 cnt;
    //char string[100];
    /* for all bits in plain text initialize whole parallel DES computer with  */
    START_COUNTER();

    for (cnt=0; cnt < LOOPS; cnt++ )
        {
            CLEAN_ALLP;
            for (j=0; j<32; j++)
            {
                FOR_64I(0);
            }
        }


    /* clean all p p[63 - i] = 0;
    for (j=0; j<32; j++)
        {
            for (i=0; i<64; i++)
            {
                if ((plaintext_p[(j << 3) + (i >> 3)] & (128 >> (i & 0x07))) != 0)
                   p[63 - i] |= (1 << j);
            }
        }
    */


    STOP_COUNTER();
    PRINT_TIME("32x 64bits 1M loops true transcoding, using macro i loop");

    printf("CT:\n");
    for (i=0; i<64; i++)
    {
        c[63 - i] = 0;
        for (j=0; j<32; j++)
        {
           if ((cryptotext_p[(j*8) + i/8] & (128 >> (i % 8))) != 0)
                c[63 - i] |= (1 << j);

        }

        //ltoa(c[63-i],string,2);
        //printf("%s\n",string);
        //printf("%d",(c[63-i]&0x01));
    }

    for (i=0; i<64; i++)
        printf("%d",(c[i]&0x01));

    printf("\n");

    if (key_p != NULL)
    {
        for (i=0; i<56; i++)
        if ((key_p[i/7] & (128 >> (i % 7))) != 0)
            k[55 - i] = ~0UL;
        else
            k[55 - i] = 0;
    }
    else
    {
        for (i=0; i<56; i++)
        k[i] = 0;
    }
}

void trial2a_test()
{
    char res = 0;
    UINT32 cnt;

	set_bitlength ();
    unroll_bits(getPlaintext(),getCryptotext(),getKey());

    START_COUNTER();
    //for (cnt=0; cnt < LOOPS; cnt++ )
        desencrypt(p,k);
    STOP_COUNTER();
    PRINT_TIME("32x 64bits, 1M loops true encryption");

    res = desverify(c);
    printf ("result = %d \n\n", res );
}

