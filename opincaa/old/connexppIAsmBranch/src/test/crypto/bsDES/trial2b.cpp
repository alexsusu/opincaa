
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


#define FOR_4ALLJ(ii,jj)\
    FOR_ALLJ(ii,jj);FOR_ALLJ(ii,(jj)+1);FOR_ALLJ(ii,(jj)+2);FOR_ALLJ(ii,(jj)+3);

#define FOR_ALLJ(ii,jj)\
    if ((plaintext_p[((jj) << 3) + ((ii) >> 3)] & (128 >> ((ii) & 0x07))) != 0)\
    p[63 - (ii)] |= (1 << (jj));

#define FOR_32ALLJ(ii)\
    FOR_4ALLJ(ii,0);FOR_4ALLJ(ii,4);FOR_4ALLJ(ii,8);FOR_4ALLJ(ii,12);\
    FOR_4ALLJ(ii,16);FOR_4ALLJ(ii,20);FOR_4ALLJ(ii,24);FOR_4ALLJ(ii,28);

#define FOR_8IJ(ii)\
    FOR_32ALLJ((ii));FOR_32ALLJ((ii)+1);FOR_32ALLJ((ii)+2);FOR_32ALLJ((ii)+3);FOR_32ALLJ((ii)+4);FOR_32ALLJ((ii)+5);FOR_32ALLJ((ii)+6);FOR_32ALLJ((ii)+7);

#define FOR_64IJ(ii)\
    FOR_8IJ((ii));FOR_8IJ((ii)+8);FOR_8IJ((ii)+16);FOR_8IJ((ii)+24);FOR_8IJ((ii)+32);FOR_8IJ((ii)+40);FOR_8IJ((ii)+48);FOR_8IJ((ii)+56);

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
            FOR_64IJ(0);
        }


    /*
        for (cnt=0; cnt < LOOPS; cnt++ )
        {
            for (i=0; i<64; i++)
            {
                p[63 - i] = 0;
                for (j=0; j<32; j++)
                    if ((plaintext_p[(j << 3) + (i >> 3)] & (128 >> (i & 0x07))) != 0)
                       p[63 - i] |= (1 << j);
            }
        }

    */

    STOP_COUNTER();
    PRINT_TIME("32x 64bits 1M loops true transcoding, using macro i*j loop");

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
    }

    if (key_p != NULL)
    {
        for (i=0; i<56; i++)
        {
            if ((key_p[i/7] & (128 >> (i % 7))) != 0)
                k[55 - i] = ~0UL;
            else
                k[55 - i] = 0;

            printf("%d", k[55-i] & 0x01);
        }
        printf("\n");
    }
    else
    {
        for (i=0; i<56; i++)
        k[i] = 0;
    }
}

void trial2b_test()
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

