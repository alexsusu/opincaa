
/*
 *
 * File: basic_match_tests_neon.cpp
 *
 * Basic_match_tests tests.
 * Computes distance between two images' descriptors, as d = Sum[(xi - xj)^2 +(yi-yj)^2 ...]
 *
 *
 */
#include "../../include/core/cnxvector_registers.h"
#include "../../include/core/cnxvector.h"
#include "../../include/core/io_unit.h"
#include "../../include/c_simu/c_simulator.h"
#include "../../include/util/utils.h"
#include "../../include/util/timing.h"
#include "../../include/util/kernel_acc.h"

#include <stdio.h>

//#include "../../include/core/neonvssse.h"

//#include <unistd.h>     /* Symbolic Constants */
//#include <sys/types.h>  /* Primitive System Data Types */
//#include <errno.h>      /* Errors */
//#include <stdio.h>      /* Input/Output */
//#include <sys/wait.h>   /* Wait for Process Termination */
//#include <stdlib.h>

#include <iostream>
#include <iomanip>

#ifndef UINT_MAX
    #define UINT_MAX ((UINT32)-1)
#endif // UINT_MAX
using namespace std;

/* About descriptor file:

In the most simple form sift takes an image in PGM format and computes its SIFT keypoints and the relative descriptors,
producing a ‘.key’ ASCII file. This file has one line per keypoint, with the x and y coordinates (pixels), the scale (pixels),
the orientation (radians) and 128 numbers (in the range 0-255) representing the descriptor.
This file is almost equivalent to the output of D. Lowe’s original implementation, except that x and y are swapped
and the orientation is negated due to a different choice of the image coordinate system. --floating-point can be used
to save full floating point descriptors instead of integer descriptors.
*/

#define FEATURES_PER_DESCRIPTOR 128
struct SiftDescriptor
{
    float X;
    float Y;
    float Scale;
    float Orientation;
};

#define MAX_DESCRIPTORS 16384
#define MAX_MATCHES (100*1000)
#define MAX_REDUCES (64*1024* 1024)

#define FACTOR2_VAL (1 << FACTOR2)

struct SiftDescriptorsF32
{
    FLOAT32 SiftDescriptorsBasicFeatures[MAX_DESCRIPTORS][FEATURES_PER_DESCRIPTOR];
    SiftDescriptor SD[MAX_DESCRIPTORS];
    UINT16 RealDescriptors;
};

struct SiftDescriptors16
{
    UINT16 SiftDescriptorsBasicFeatures[MAX_DESCRIPTORS][FEATURES_PER_DESCRIPTOR];
    SiftDescriptor SD[MAX_DESCRIPTORS];
    UINT16 RealDescriptors;
};

struct SiftDescriptors8
{
    UINT8 SiftDescriptorsBasicFeatures[MAX_DESCRIPTORS][FEATURES_PER_DESCRIPTOR];
    SiftDescriptor SD[MAX_DESCRIPTORS];
    UINT16 RealDescriptors;
};

struct SiftMatches
{
    //DescriptorIndexInSecondImage
    UINT32 DescIx2ndImgMin[MAX_MATCHES];
    UINT32 DescIx2ndImgNextToMin[MAX_MATCHES];
    UINT32 ScoreMin[MAX_MATCHES];
    UINT32 ScoreNextToMin[MAX_MATCHES];
    UINT16 RealMatches;
};

/* only UINT16 is implemented on Connex-S */
#ifndef __ARM_NEON__
static SiftDescriptorsF32 SiftDescriptorsF32_1 __attribute__ ((aligned(32)));
static SiftDescriptorsF32 SiftDescriptorsF32_2 __attribute__ ((aligned(32)));

static SiftDescriptors16 SiftDescriptors16_1 __attribute__ ((aligned(32)));
static SiftDescriptors16 SiftDescriptors16_2 __attribute__ ((aligned(32)));

static SiftDescriptors8 SiftDescriptors8_1 __attribute__ ((aligned(32)));
static SiftDescriptors8 SiftDescriptors8_2 __attribute__ ((aligned(32)));
#endif

static SiftMatches SM_Arm;
static SiftMatches SM_Arm_SSE;
//static SiftMatches SM_ConnexArm;
//static SiftMatches SM_ConnexArmMan;
//static UINT_RED_REG_VAL BasicMatchRedResults[MAX_REDUCES];

static void PrintDescriptors(SiftDescriptors16 *SDs)
{
    int DescriptorIndex=0, FeatIndex;
    for(DescriptorIndex =0; DescriptorIndex < SDs->RealDescriptors; DescriptorIndex++)
    //for(DescriptorIndex =0; DescriptorIndex < 2; DescriptorIndex++)
    {
        printf("SDs[%d]->SD.X = %f\n",DescriptorIndex,SDs->SD[DescriptorIndex].X);
        printf("SDs[%d]->SD.Y = %f\n",DescriptorIndex,SDs->SD[DescriptorIndex].Y);
        printf("SDs[%d]->SD.S = %f\n",DescriptorIndex,SDs->SD[DescriptorIndex].Scale);
        printf("SDs[%d]->SD.O = %f\n",DescriptorIndex,SDs->SD[DescriptorIndex].Orientation);

        for (FeatIndex = 0; FeatIndex < FEATURES_PER_DESCRIPTOR; FeatIndex++)
        //for (FeatIndex = 0; FeatIndex < 1; FeatIndex++)
            printf("SDs[%d]->SD.BasicFeatures[%d] = %d \n",
                   DescriptorIndex,
                   FeatIndex,
                   SDs->SiftDescriptorsBasicFeatures[DescriptorIndex][FeatIndex]);
    }
}

static void PrintMatches(SiftMatches *SMs)
{
    int MatchesIndex=0;
    printf("\n");
    for(MatchesIndex =0; MatchesIndex < SMs->RealMatches; MatchesIndex++)
    //for(MatchesIndex =0; MatchesIndex < 10; MatchesIndex++)
    {
        printf("Matches[%d].DescriptorIndexInSecondImage = %d \n", MatchesIndex, SMs->DescIx2ndImgMin[MatchesIndex]);
        /*
        printf("Matches[%d].X1,Y1,X2,Y2,Sc= %f %f %f %f %d\n",
               MatchesIndex, SMs->Matches[MatchesIndex].X1, SMs->Matches[MatchesIndex].Y1,
               SMs->Matches[MatchesIndex].X2,SMs->Matches[MatchesIndex].Y2, SMs->Matches[MatchesIndex].Score);
        */
    }
}

static int CompareMatches(SiftMatches *SMs1, SiftMatches *SMs2)
{
    int CntMax = SMs1->RealMatches;
    cout << "Comparing "<<CntMax<< " matches... "<<endl;
    if (CntMax != SMs2->RealMatches) {cout << "FAIL: not the same number of matches "<<SMs1->RealMatches<<" != "<<SMs2->RealMatches <<endl;return FAIL;}
    for (int cnt = 0; cnt < CntMax; cnt++)
        if (SMs1->DescIx2ndImgMin[cnt] != SMs2->DescIx2ndImgMin[cnt])
            {
                cout <<"Failed at index "<<cnt<<endl;
                return FAIL;
            }

    return PASS;
}

static int LoadDescriptorsF32(char *FileName, SiftDescriptorsF32 *SDs, int Limit)
{
    FILE *fp;
    int Start = GetMilliCount();
    if((fp = fopen(FileName, "r")) == NULL)
    {
        printf("No such file\n");
        exit(1);
    }
    else
    {
        int FeatIndex;
        int DescriptorIndex=0;
        int result;
        SDs->RealDescriptors = 0;

        while(1)
        {
            fscanf(fp,"%f",&SDs->SD[DescriptorIndex].X);
            fscanf(fp,"%f",&SDs->SD[DescriptorIndex].Y);
            fscanf(fp,"%f",&SDs->SD[DescriptorIndex].Scale);
            fscanf(fp,"%f",&SDs->SD[DescriptorIndex].Orientation);

            for (FeatIndex = 0; FeatIndex < FEATURES_PER_DESCRIPTOR; FeatIndex++)
            {
                int val;
                result = fscanf(fp,"%d",&val);
                SDs->SiftDescriptorsBasicFeatures[DescriptorIndex][FeatIndex] = val;
            }

            if (result == 1) SDs->RealDescriptors++;
            else
                {
                    fclose(fp);
                    //printf("Reached end of file\n");
                    printf("Loading of all %d descriptors took = %d ms \n", SDs->RealDescriptors,GetMilliSpan(Start));
                    return 0;
                }

            DescriptorIndex++;
            if (Limit !=0)
                if (DescriptorIndex == Limit)
                {
                    fclose(fp);
                    //printf("Reached end of file\n");
                    printf("LimitLoading of %d descriptors took = %d ms \n", Limit,GetMilliSpan(Start));
                    return 0;
                }
        }
    }
}

static int LoadDescriptors16(char *FileName, SiftDescriptors16 *SDs, int Limit)
{
    FILE *fp;
    int Start = GetMilliCount();
    if((fp = fopen(FileName, "r")) == NULL)
    {
        printf("No such file\n");
        exit(1);
    }
    else
    {
        int FeatIndex;
        int DescriptorIndex=0;
        int result;
        SDs->RealDescriptors = 0;

        while(1)
        {
            fscanf(fp,"%f",&SDs->SD[DescriptorIndex].X);
            fscanf(fp,"%f",&SDs->SD[DescriptorIndex].Y);
            fscanf(fp,"%f",&SDs->SD[DescriptorIndex].Scale);
            fscanf(fp,"%f",&SDs->SD[DescriptorIndex].Orientation);

            for (FeatIndex = 0; FeatIndex < FEATURES_PER_DESCRIPTOR; FeatIndex++)
            {
                int val;
                result = fscanf(fp,"%d",&val);
                SDs->SiftDescriptorsBasicFeatures[DescriptorIndex][FeatIndex] = val;
            }


            if (result == 1) SDs->RealDescriptors++;
            else
                {
                    fclose(fp);
                    //printf("Reached end of file\n");
                    printf("Loading of all %d descriptors took = %d ms \n", SDs->RealDescriptors,GetMilliSpan(Start));
                    return 0;
                }

            DescriptorIndex++;
            if (Limit !=0)
                if (DescriptorIndex == Limit)
                {
                    fclose(fp);
                    //printf("Reached end of file\n");
                    printf("LimitLoading of %d descriptors took = %d ms \n", Limit,GetMilliSpan(Start));
                    return 0;
                }
        }
    }
}


static int LoadDescriptors8(char *FileName, SiftDescriptors8 *SDs, int Limit)
{
    FILE *fp;
    int Start = GetMilliCount();
    if((fp = fopen(FileName, "r")) == NULL)
    {
        printf("No such file\n");
        exit(1);
    }
    else
    {
        int FeatIndex;
        int DescriptorIndex=0;
        int result;
        SDs->RealDescriptors = 0;

        while(1)
        {
            fscanf(fp,"%f",&SDs->SD[DescriptorIndex].X);
            fscanf(fp,"%f",&SDs->SD[DescriptorIndex].Y);
            fscanf(fp,"%f",&SDs->SD[DescriptorIndex].Scale);
            fscanf(fp,"%f",&SDs->SD[DescriptorIndex].Orientation);

            for (FeatIndex = 0; FeatIndex < FEATURES_PER_DESCRIPTOR; FeatIndex++)
            {
                int val;
                result = fscanf(fp,"%d",&val);
                SDs->SiftDescriptorsBasicFeatures[DescriptorIndex][FeatIndex] = val;
            }

            if (result == 1) SDs->RealDescriptors++;
            else
                {
                    fclose(fp);
                    //printf("Reached end of file\n");
                    printf("Loading of all %d descriptors took = %d ms \n", SDs->RealDescriptors,GetMilliSpan(Start));
                    return 0;
                }

            DescriptorIndex++;
            if (Limit !=0)
                if (DescriptorIndex == Limit)
                {
                    fclose(fp);
                    //printf("Reached end of file\n");
                    printf("LimitLoading of %d descriptors took = %d ms \n", Limit,GetMilliSpan(Start));
                    return 0;
                }
        }
    }
}


#define FACTOR1 46
#define FACTOR2 7 // that is (1 << 7)
/* We have to compare x/y with 0.36 ; if it is less, we have a true match

x/y < 0.36 is eqv with x < 0.36*y
0.36*y ~ 0.359375 *y = ((46 * y) >> 7) = (FACTOR1 * y) >> FACTOR2

*/
static void SSD_FindMatches16(SiftDescriptors16 *SDs1, SiftDescriptors16 *SDs2, SiftMatches* SMs)
{
    SMs->RealMatches = 0;
    int DescriptorIndex1;
	for (DescriptorIndex1 =0; DescriptorIndex1 < SDs1->RealDescriptors; DescriptorIndex1++)
    {
		int DescriptorIndex2;
		int minIndex = 0;
		//int nexttominIndex;
		UINT32 distsq1, distsq2;
		distsq1 = (UINT32)-1;
		distsq2 = (UINT32)-1;

        for (DescriptorIndex2 =0; DescriptorIndex2 < SDs2->RealDescriptors; DescriptorIndex2++)
	    {
            UINT32 dsq = 0;
            for (int FeatIndex = 0; FeatIndex < FEATURES_PER_DESCRIPTOR; FeatIndex++)
            {
                dsq += (SDs1->SiftDescriptorsBasicFeatures[DescriptorIndex1][FeatIndex] - SDs2->SiftDescriptorsBasicFeatures[DescriptorIndex2][FeatIndex])
                        *(SDs1->SiftDescriptorsBasicFeatures[DescriptorIndex1][FeatIndex] - SDs2->SiftDescriptorsBasicFeatures[DescriptorIndex2][FeatIndex]);
            }

            if (dsq < distsq1)
                {
                    distsq2 = distsq1;
                    distsq1 = dsq;
                    //nexttominIndex = minIndex;
                    minIndex = DescriptorIndex2;
                }
            else if (dsq < distsq2)
                {
                    distsq2 = dsq;
                    //nexttominIndex = DescriptorIndex2;
                }
        }
        if (distsq1 < (FACTOR1 * distsq2) >> FACTOR2)
            SMs->DescIx2ndImgMin[SMs->RealMatches++] = minIndex;
        //otherwise overwrite it next image1 descriptor
    }
}

static void SAD_FindMatches8(SiftDescriptors8 *SDs1, SiftDescriptors8 *SDs2, SiftMatches* SMs)
{
    SMs->RealMatches = 0;
    int DescriptorIndex1;

	for (DescriptorIndex1 =0; DescriptorIndex1 < SDs1->RealDescriptors; DescriptorIndex1++)
    {
		int DescriptorIndex2;
		int minIndex = 0;
		//int nexttominIndex;
		UINT32 distsq1, distsq2;
		distsq1 = (UINT32)-1;
		distsq2 = (UINT32)-1;

        for (DescriptorIndex2 =0; DescriptorIndex2 < SDs2->RealDescriptors; DescriptorIndex2++)
	    {
            UINT32 dsq = 0;
            for (int FeatIndex = 0; FeatIndex < FEATURES_PER_DESCRIPTOR; FeatIndex++)
            {
                dsq += abs(SDs1->SiftDescriptorsBasicFeatures[DescriptorIndex1][FeatIndex] -
                            SDs2->SiftDescriptorsBasicFeatures[DescriptorIndex2][FeatIndex]);
            }

            if (dsq < distsq1)
                {
                    distsq2 = distsq1;
                    distsq1 = dsq;
                    //nexttominIndex = minIndex;
                    minIndex = DescriptorIndex2;
                }
            else if (dsq < distsq2)
                {
                    distsq2 = dsq;
                    //nexttominIndex = DescriptorIndex2;
                }
        }
        if (distsq1 < (FACTOR1 * distsq2) >> FACTOR2)
            SMs->DescIx2ndImgMin[SMs->RealMatches++] = minIndex;
        //otherwise overwrite it next image1 descriptor
    }
}

static void SAD_FindMatches16(SiftDescriptors16 *SDs1, SiftDescriptors16 *SDs2, SiftMatches* SMs)
{
    SMs->RealMatches = 0;
    int DescriptorIndex1;

	for (DescriptorIndex1 =0; DescriptorIndex1 < SDs1->RealDescriptors; DescriptorIndex1++)
    {
		int DescriptorIndex2;
		int minIndex = 0;
		//int nexttominIndex;
		UINT32 distsq1, distsq2;
		distsq1 = (UINT32)-1;
		distsq2 = (UINT32)-1;

        for (DescriptorIndex2 =0; DescriptorIndex2 < SDs2->RealDescriptors; DescriptorIndex2++)
	    {
            UINT32 dsq = 0;
            for (int FeatIndex = 0; FeatIndex < FEATURES_PER_DESCRIPTOR; FeatIndex++)
            {
                dsq += abs(SDs1->SiftDescriptorsBasicFeatures[DescriptorIndex1][FeatIndex] -
                            SDs2->SiftDescriptorsBasicFeatures[DescriptorIndex2][FeatIndex]);
            }

            if (dsq < distsq1)
                {
                    distsq2 = distsq1;
                    distsq1 = dsq;
                    //nexttominIndex = minIndex;
                    minIndex = DescriptorIndex2;
                }
            else if (dsq < distsq2)
                {
                    distsq2 = dsq;
                    //nexttominIndex = DescriptorIndex2;
                }
        }
        if (distsq1 < (FACTOR1 * distsq2) >> FACTOR2)
            SMs->DescIx2ndImgMin[SMs->RealMatches++] = minIndex;
        //otherwise overwrite it next image1 descriptor
    }
}

static void SAD_FindMatchesF32(SiftDescriptorsF32* SDs1, SiftDescriptorsF32* SDs2, SiftMatches* SMs)
{
    SMs->RealMatches = 0;
    for (int DescriptorIndex1 =0; DescriptorIndex1 < SDs1->RealDescriptors; DescriptorIndex1++)
    {
		int minIndex = 0;
		//int nexttominIndex;
		FLOAT32 distsq1, distsq2;
		distsq1 = (FLOAT32)UINT_MAX;
		distsq2 = (FLOAT32)UINT_MAX;

        for (int DescriptorIndex2 =0; DescriptorIndex2 < SDs2->RealDescriptors; DescriptorIndex2++)
	    {
            FLOAT32 dsq = 0;
            for (int FeatIndex = 0; FeatIndex < FEATURES_PER_DESCRIPTOR; FeatIndex++)
                dsq += abs(SDs1->SiftDescriptorsBasicFeatures[DescriptorIndex1][FeatIndex] -
                                    SDs2->SiftDescriptorsBasicFeatures[DescriptorIndex2][FeatIndex]);

            if (dsq < distsq1)
                {
                    distsq2 = distsq1;
                    distsq1 = dsq;
                    //nexttominIndex = minIndex;
                    minIndex = DescriptorIndex2;
                }
            else if (dsq < distsq2)
                {
                    distsq2 = dsq;
                    //nexttominIndex = DescriptorIndex2;
                }
        }
        if (distsq1 < (FACTOR1 * distsq2) / FACTOR2_VAL)
            SMs->DescIx2ndImgMin[SMs->RealMatches++] = minIndex;
        //otherwise overwrite it next image1 descriptor
    }
}

static void SSD_FindMatchesF32(SiftDescriptorsF32 *SDs1, SiftDescriptorsF32 *SDs2, SiftMatches* SMs)
{
    SMs->RealMatches = 0;
    for (int DescriptorIndex1 =0; DescriptorIndex1 < SDs1->RealDescriptors; DescriptorIndex1++)
    {
		int minIndex = 0;
		//int nexttominIndex;
		FLOAT32 distsq1, distsq2;
		distsq1 = (FLOAT32)UINT_MAX;
		distsq2 = (FLOAT32)UINT_MAX;

        for (int DescriptorIndex2 =0; DescriptorIndex2 < SDs2->RealDescriptors; DescriptorIndex2++)
	    {
            FLOAT32 dsq = 0;
            for (int FeatIndex = 0; FeatIndex < FEATURES_PER_DESCRIPTOR; FeatIndex++)
                dsq += (SDs1->SiftDescriptorsBasicFeatures[DescriptorIndex1][FeatIndex] - SDs2->SiftDescriptorsBasicFeatures[DescriptorIndex2][FeatIndex])
                        *(SDs1->SiftDescriptorsBasicFeatures[DescriptorIndex1][FeatIndex] - SDs2->SiftDescriptorsBasicFeatures[DescriptorIndex2][FeatIndex]);

            if (dsq < distsq1)
                {
                    distsq2 = distsq1;
                    distsq1 = dsq;
                    //nexttominIndex = minIndex;
                    minIndex = DescriptorIndex2;
                }
            else if (dsq < distsq2)
                {
                    distsq2 = dsq;
                    //nexttominIndex = DescriptorIndex2;
                }
        }
        if (distsq1 < (FACTOR1 * distsq2) / FACTOR2_VAL)
            SMs->DescIx2ndImgMin[SMs->RealMatches++] = minIndex;
        //otherwise overwrite it next image1 descriptor
    }
}

/*
    Out of 1024 LocalStore locations, each having 128 Bytes:
    (fortunately a SIFT descriptor also has 128 features of 1 Bytes each)
*/
#define VECTORS_CHUNK_IMAGE1 364  //  364 cnxvectors - reserved for img1 (cnxvectorS_CHUNK_IMG1)
#define VECTORS_CHUNK_IMAGE2 330 // 330 cnxvectors - reserved for img2 - work (cnxvectorS_CHUNK_IMG2)
//another 330 cnxvectors - reserved for img2 - transfer with IO

#define VECTORS_SUBCHUNK_IMAGE2 30 // 30 cnxvectors from VECTORS_CHUNK_IMAGE2 to be cached for inner loop of distance calculation

#define BASIC_MATCHING_BNR 0

//SSSE3: needed for _mm_maddubs
#ifdef __ARM_NEON__
    #include <arm_neon.h>
#else //SSE
    //#include <tmmintrin.h>

    //AVX:
    #include <immintrin.h>

    //extern void printv(__m128 m);

    #define cpuid(func,ax,bx,cx,dx)\
        __asm__ __volatile__ ("cpuid":\
        "=a" (ax), "=b" (bx), "=c" (cx), "=d" (dx) : "a" (func));

    #define INT8 char
    #define XMM_DIFF(x,y) x = _mm_sub_epi8(x, y)
    #define XMM_ABS(x) x = _mm_abs_epi8(x)
    #define XMM_MULT_REDUCTION_ADD(x) x =  _mm_maddubs_epi16(x, x)
#endif // __ARM_NEON__


#define LOAD_128_bits _mm_load_si128
//#define LOAD_128_bits _mm_lddqu_si128
//#define LOAD_128_bits _mm_loadu_si128

#define LOAD_F32_256_bits  _mm256_load_ps


//#define __ARM_NEON__
#ifdef __ARM_NEON__
static void SSD_FindMatches16_SSE(SiftDescriptors16 *SDs1, SiftDescriptors16 *SDs2, SiftMatches* SMs) {};
/*
static void SSD_FindMatches16_SSE(SiftDescriptors16 *SDs1, SiftDescriptors16 *SDs2, SiftMatches* SMs)
{
    SMs->RealMatches = 0;
    int DescriptorIndex1;
	for (DescriptorIndex1 =0; DescriptorIndex1 < SDs1->RealDescriptors; DescriptorIndex1++)
    {
		int DescriptorIndex2;
		int minIndex;
		int nexttominIndex;
		UINT32 dsq, distsq1, distsq2;
		distsq1 = (UINT32)-1;
		distsq2 = (UINT32)-1;

        //load 128 bits as 16x 8 bits. Optimized for cache line of 64 Bytes = 512 bits (Intel SandyBridge)

            uint8x16x2_t point1[4] __attribute__ ((aligned16));
            uint8x16x2_t *src1 = (uint8x16x2_t*)__builtin_assume_aligned((SDs1->SiftDescriptorsBasicFeatures[DescriptorIndex1]), 16);
            //load 128 Bytes of data (8 x (16x8) bits )
            point1[0] = vld2q_u8(src1[0]);
            point1[1] = vld2q_u8(src1[1]);
            point1[2] = vld2q_u8(src1[2]);
            point1[3] = vld2q_u8(src1[3]);

        for (DescriptorIndex2 =0; DescriptorIndex2 < SDs2->RealDescriptors; DescriptorIndex2++)
	    {
	        dsq = 0;
            uint8x16x2_t point2[4] __attribute__ ((aligned16));

            uint8x16x2_t *src2 = (uint8x16x2_t*)__builtin_assume_aligned((SDs2->SiftDescriptorsBasicFeatures[DescriptorIndex2]), 16);

            //load 128 Bytes of data (8 x (16x8) bits )
            point2[0] = vld2q_u8(src2[0]);
            point2[1] = vld2q_u8(src2[1]);
            point2[2] = vld2q_u8(src2[2]);
            point2[3] = vld2q_u8(src2[3]);

            //uint8x8_t   vabd_u8(uint8x8_t a, uint8x8_t b);
            point2[0].val[0] = vabd_u8(point2[0].val[0], point1[0].val[0]);
            point2[0].val[1] = vabd_u8(point2[0].val[1], point1[0].val[1]);

            point2[1].val[0] = vabd_u8(point2[1].val[0], point1[1].val[0]);
            point2[1].val[1] = vabd_u8(point2[1].val[1], point1[1].val[1]);

            point2[2].val[0] = vabd_u8(point2[2].val[0], point1[2].val[0]);
            point2[2].val[1] = vabd_u8(point2[2].val[1], point1[2].val[1]);

            point2[3].val[0] = vabd_u8(point2[3].val[0], point1[3].val[0]);
            point2[3].val[1] = vabd_u8(point2[3].val[1], point1[3].val[1]);

            uint16x8x2_t point3[8] __attribute__ ((aligned16));
            //set to zero !

            point3[0].val[0] = vmlal_u8(point3[0].val[0], point2[0].val[0], point2[0].val[0]);
            point3[0].val[1] = vmlal_u8(point3[0].val[1], point2[0].val[1], point2[0].val[1]);

            point3[1].val[0] = vmlal_u8(point3[1].val[0], point2[2].val[0], point2[1].val[0]);
            point3[1].val[1] = vmlal_u8(point3[1].val[1], point2[2].val[1], point2[1].val[1]);

            point3[2].val[0] = vmlal_u8(point3[2].val[0], point2[3].val[0], point2[2].val[0]);
            point3[2].val[1] = vmlal_u8(point3[2].val[1], point2[3].val[1], point2[2].val[1]);

            point3[3].val[0] = vmlal_u8(point3[3].val[0], point2[3].val[0], point2[3].val[0]);
            point3[3].val[1] = vmlal_u8(point3[3].val[1], point2[3].val[1], point2[3].val[1]);

            UINT16 mults[8] __attribute__ ((aligned16));

            vst1q_u16(&mults[0], point3[0].val[0]);
            vst1q_u16(&mults[1], point3[0].val[1]);
            vst1q_u16(&mults[2], point3[1].val[0]);
            vst1q_u16(&mults[3], point3[1].val[0]);

            vst1q_u16(&mults[4], point3[2].val[0]);
            vst1q_u16(&mults[5], point3[2].val[1]);
            vst1q_u16(&mults[6], point3[3].val[0]);
            vst1q_u16(&mults[7], point3[3].val[1]);

            for (int i = 0; i < 8; i++)
                dsq += mults[i];

            if (dsq < distsq1)
                {
                    distsq2 = distsq1;
                    distsq1 = dsq;
                    nexttominIndex = minIndex;
                    minIndex = DescriptorIndex2;
                }
            else if (dsq < distsq2)
                {
                    distsq2 = dsq;
                    nexttominIndex = DescriptorIndex2;
                }
        }
        if (distsq1 < (FACTOR1 * distsq2) >> FACTOR2)
            SMs->DescIx2ndImgMin[SMs->RealMatches++] = minIndex;
        //otherwise overwrite it next image1 descriptor
    }
}
*/
#else

/*
    It is pushing the limit of the SSE register count.
    It uses "using" 17 registers out of the 16 existing.
*/
static void SSD_FindMatches16_SSE(SiftDescriptors16 *SDs1, SiftDescriptors16 *SDs2, SiftMatches* SMs)
{
    SMs->RealMatches = 0;
    int DescriptorIndex1;
    INT32 multsI32[4*16] __attribute__ ((aligned(16)));

	for (DescriptorIndex1 =0; DescriptorIndex1 < SDs1->RealDescriptors; DescriptorIndex1++)
    {
		int DescriptorIndex2;
		int minIndex = 0;
		//int nexttominIndex;
		UINT32 dsq, distsq1, distsq2;
		distsq1 = (UINT32)-1;
		distsq2 = (UINT32)-1;

            //load 128 bits as 8x 16 bits. Optimized for cache line of 64 Bytes = 512 bits (Intel SandyBridge)
            INT16 *src1 = (INT16*)__builtin_assume_aligned((SDs1->SiftDescriptorsBasicFeatures[DescriptorIndex1]), 16);
            //load 128 Bytes of data (8 x (16x8) bits )
            #define LOAD_SSD_FindMatches16_SSE(x) __m128i m##x = LOAD_128_bits((__m128i*)(src1 + 8 * x));
            LOAD_SSD_FindMatches16_SSE(0);LOAD_SSD_FindMatches16_SSE(1);LOAD_SSD_FindMatches16_SSE(2);LOAD_SSD_FindMatches16_SSE(3);
            LOAD_SSD_FindMatches16_SSE(4);LOAD_SSD_FindMatches16_SSE(5);LOAD_SSD_FindMatches16_SSE(6);LOAD_SSD_FindMatches16_SSE(7);
            LOAD_SSD_FindMatches16_SSE(8);LOAD_SSD_FindMatches16_SSE(9);LOAD_SSD_FindMatches16_SSE(10);LOAD_SSD_FindMatches16_SSE(11);
            LOAD_SSD_FindMatches16_SSE(12);LOAD_SSD_FindMatches16_SSE(13);LOAD_SSD_FindMatches16_SSE(14);LOAD_SSD_FindMatches16_SSE(15);

        for (DescriptorIndex2 =0; DescriptorIndex2 < SDs2->RealDescriptors; DescriptorIndex2++)
	    {
	        dsq = 0;
            INT16 *src2 = (INT16*)__builtin_assume_aligned((SDs2->SiftDescriptorsBasicFeatures[DescriptorIndex2]), 16);
            __m128i m16;
            #define PARTIAL_SSD(x) \
            m16 = LOAD_128_bits((__m128i*)(src2+x*8));\
            m16 = _mm_sub_epi16(m16,m##x);\
            m16 = _mm_madd_epi16(m16,m16);\
            _mm_store_si128((__m128i*)(multsI32+x*4), m16);

            PARTIAL_SSD(0);            PARTIAL_SSD(1);            PARTIAL_SSD(2);            PARTIAL_SSD(3);
            PARTIAL_SSD(4);            PARTIAL_SSD(5);            PARTIAL_SSD(6);            PARTIAL_SSD(7);
            PARTIAL_SSD(8);            PARTIAL_SSD(9);            PARTIAL_SSD(10);           PARTIAL_SSD(11);
            PARTIAL_SSD(12);           PARTIAL_SSD(13);           PARTIAL_SSD(14);           PARTIAL_SSD(15);

            for (int i = 0; i < 64; i++)
                dsq += multsI32[i];

            if (dsq < distsq1)
                {
                    distsq2 = distsq1;
                    distsq1 = dsq;
                    //nexttominIndex = minIndex;
                    minIndex = DescriptorIndex2;
                }
            else if (dsq < distsq2)
                {
                    distsq2 = dsq;
                    //nexttominIndex = DescriptorIndex2;
                }
        }
        if (distsq1 < (FACTOR1 * distsq2) >> FACTOR2)
            SMs->DescIx2ndImgMin[SMs->RealMatches++] = minIndex;
        //otherwise overwrite it next image1 descriptor
    }
}
#endif // __SSE__

#ifdef __ARM_NEON__
static void SSD_FindMatchesF32_SSE(SiftDescriptorsF32 *SDs1, SiftDescriptorsF32 *SDs2, SiftMatches* SMs) {};
#else
static void SSD_FindMatchesF32_SSE(SiftDescriptorsF32 *SDs1, SiftDescriptorsF32 *SDs2, SiftMatches* SMs)
{
    SMs->RealMatches = 0;
    int DescriptorIndex1;
    FLOAT32 multsF32[128] __attribute__ ((aligned(32)));

	for (DescriptorIndex1 =0; DescriptorIndex1 < SDs1->RealDescriptors; DescriptorIndex1++)
    {
		int DescriptorIndex2;
		int minIndex = 0;
		//int nexttominIndex;
		FLOAT32 dsq, distsq1, distsq2;
        distsq1 = (FLOAT32)UINT_MAX;
		distsq2 = (FLOAT32)UINT_MAX;

            FLOAT32 *src1 = (FLOAT32*)__builtin_assume_aligned((SDs1->SiftDescriptorsBasicFeatures[DescriptorIndex1]), 32);
            //load 128 Bytes of data (8 x (16x8) bits )

            #define LOAD_32F_SSD(y) __m256 m##y = LOAD_F32_256_bits(src1 + y*8);
            LOAD_32F_SSD(0);LOAD_32F_SSD(1);LOAD_32F_SSD(2);LOAD_32F_SSD(3);
            LOAD_32F_SSD(4);LOAD_32F_SSD(5);LOAD_32F_SSD(6);LOAD_32F_SSD(7);
            LOAD_32F_SSD(8);LOAD_32F_SSD(9);LOAD_32F_SSD(10);LOAD_32F_SSD(11);
            LOAD_32F_SSD(12);LOAD_32F_SSD(13);LOAD_32F_SSD(14);LOAD_32F_SSD(15);

        for (DescriptorIndex2 =0; DescriptorIndex2 < SDs2->RealDescriptors; DescriptorIndex2++)
	    {
	        dsq = 0;
            FLOAT32 *src2 = (FLOAT32*)__builtin_assume_aligned((SDs2->SiftDescriptorsBasicFeatures[DescriptorIndex2]), 32);
            __m256 m16;
            #define PARTIAL_SSD_F32(x) \
            m16 = LOAD_F32_256_bits(src2 + x*8);\
            m16 = _mm256_sub_ps(m16,m##x);\
            m16 = _mm256_mul_ps(m16,m16);\
            m16 = _mm256_hadd_ps(m16,m16);\
             _mm256_store_ps(multsF32 + x*8, m16);

            PARTIAL_SSD_F32(0);            PARTIAL_SSD_F32(1);            PARTIAL_SSD_F32(2);            PARTIAL_SSD_F32(3);
            PARTIAL_SSD_F32(4);            PARTIAL_SSD_F32(5);            PARTIAL_SSD_F32(6);            PARTIAL_SSD_F32(7);
            PARTIAL_SSD_F32(8);            PARTIAL_SSD_F32(9);            PARTIAL_SSD_F32(10);           PARTIAL_SSD_F32(11);
            PARTIAL_SSD_F32(12);           PARTIAL_SSD_F32(13);           PARTIAL_SSD_F32(14);           PARTIAL_SSD_F32(15);

            for (int i = 0; i < 128; i+=4) dsq += multsF32[i] + multsF32[i+1];

            if (dsq < distsq1)
                {
                    distsq2 = distsq1;
                    distsq1 = dsq;
                    //nexttominIndex = minIndex;
                    minIndex = DescriptorIndex2;
                }
            else if (dsq < distsq2)
                {
                    distsq2 = dsq;
                    //nexttominIndex = DescriptorIndex2;
                }
        }
        if (distsq1 < (FACTOR1 * distsq2) / FACTOR2_VAL)
            SMs->DescIx2ndImgMin[SMs->RealMatches++] = minIndex;
        //otherwise overwrite it next image1 descriptor
    }
}
#endif // __SSE__

#ifdef __ARM_NEON__
static void SAD_FindMatchesSSE(SiftDescriptors8 *SDs1, SiftDescriptors8 *SDs2, SiftMatches* SMs)
{}
#else
static void SAD_FindMatches8_SSE(SiftDescriptors8 *SDs1, SiftDescriptors8 *SDs2, SiftMatches* SMs)
{
    SMs->RealMatches = 0;
    int DescriptorIndex1;
    UINT16 multsUI16[4*16] __attribute__ ((aligned(16)));

	for (DescriptorIndex1 =0; DescriptorIndex1 < SDs1->RealDescriptors; DescriptorIndex1++)
    {
		int DescriptorIndex2;
		int minIndex = 0;
		//int nexttominIndex;
		UINT32 dsq, distsq1, distsq2;
		distsq1 = (UINT32)-1;
		distsq2 = (UINT32)-1;

            //load 128 bits as 8x 16 bits. Optimized for cache line of 64 Bytes = 512 bits (Intel SandyBridge)
            UINT8 *src1 = (UINT8*)__builtin_assume_aligned((SDs1->SiftDescriptorsBasicFeatures[DescriptorIndex1]), 16);
            //load 128 Bytes of data (8 x (16x8) bits )

            __m128i m0 = LOAD_128_bits((__m128i*)(src1));
            __m128i m1 = LOAD_128_bits((__m128i*)(src1 + 16));
            __m128i m2 = LOAD_128_bits((__m128i*)(src1 + 32));
            __m128i m3 = LOAD_128_bits((__m128i*)(src1 + 48));
            __m128i m4 = LOAD_128_bits((__m128i*)(src1 + 64));
            __m128i m5 = LOAD_128_bits((__m128i*)(src1 + 80));
            __m128i m6 = LOAD_128_bits((__m128i*)(src1 + 96));
            __m128i m7 = LOAD_128_bits((__m128i*)(src1 + 112));


        for (DescriptorIndex2 =0; DescriptorIndex2 < SDs2->RealDescriptors; DescriptorIndex2++)
	    {
	        dsq = 0;
            UINT8 *src2 = (UINT8*)__builtin_assume_aligned((SDs2->SiftDescriptorsBasicFeatures[DescriptorIndex2]), 16);
            __m128i m8;
            #define PARTIAL_SAD8(x) \
            m8 = LOAD_128_bits((__m128i*)(src2+x*16));\
            m8 = _mm_sad_epu8(m8,m##x);\
            _mm_store_si128((__m128i*)(multsUI16+x*8), m8);

            PARTIAL_SAD8(0);    PARTIAL_SAD8(1);    PARTIAL_SAD8(2);    PARTIAL_SAD8(3);
            PARTIAL_SAD8(4);    PARTIAL_SAD8(5);    PARTIAL_SAD8(6);    PARTIAL_SAD8(7);

            for (int i = 0; i < 64; i+=4)
                dsq += multsUI16[i];

            if (dsq < distsq1)
                {
                    distsq2 = distsq1;
                    distsq1 = dsq;
                    //nexttominIndex = minIndex;
                    minIndex = DescriptorIndex2;
                }
            else if (dsq < distsq2)
                {
                    distsq2 = dsq;
                    //nexttominIndex = DescriptorIndex2;
                }
        }
        if (distsq1 < (FACTOR1 * distsq2) >> FACTOR2)
            SMs->DescIx2ndImgMin[SMs->RealMatches++] = minIndex;
        //otherwise overwrite it next image1 descriptor
    }
}
#endif // __SSE__


#ifdef __ARM_NEON__
static void SAD_FindMatchesSSE(SiftDescriptors16 *SDs1, SiftDescriptors16 *SDs2, SiftMatches* SMs)
{}
#else
static void SAD_FindMatches16_SSE(SiftDescriptors16 *SDs1, SiftDescriptors16 *SDs2, SiftMatches* SMs)
{
    SMs->RealMatches = 0;
    UINT16 multsU16[8*16] __attribute__ ((aligned(16)));

    int minIndex = 0;
    //int nexttominIndex;
    UINT32 dsq, distsq1, distsq2;

    for (int DescriptorIndex1 =0; DescriptorIndex1 < SDs1->RealDescriptors; DescriptorIndex1++)
    {
        distsq1 = (UINT32)-1;
        distsq2 = (UINT32)-1;

            //load 128 bits as 8x 16 bits. Optimized for cache line of 64 Bytes = 512 bits (Intel SandyBridge)
            UINT16 *src1 = (UINT16*)__builtin_assume_aligned((SDs1->SiftDescriptorsBasicFeatures[DescriptorIndex1]), 16);
            //load 128 Bytes of data (8 x (16x8) bits )
            #define LOAD_128_bits_src1_16(x) __m128i m##x = LOAD_128_bits((__m128i*)(src1 + 8*x))
            LOAD_128_bits_src1_16(0);LOAD_128_bits_src1_16(1);LOAD_128_bits_src1_16(2);LOAD_128_bits_src1_16(3);
            LOAD_128_bits_src1_16(4);LOAD_128_bits_src1_16(5);LOAD_128_bits_src1_16(6);LOAD_128_bits_src1_16(7);
            LOAD_128_bits_src1_16(8);LOAD_128_bits_src1_16(9);LOAD_128_bits_src1_16(10);LOAD_128_bits_src1_16(11);
            LOAD_128_bits_src1_16(12);LOAD_128_bits_src1_16(13);LOAD_128_bits_src1_16(14);LOAD_128_bits_src1_16(15);

        for (int DescriptorIndex2 =0; DescriptorIndex2 < SDs2->RealDescriptors; DescriptorIndex2++)
	    {
	        dsq = 0;
            UINT16 *src2 = (UINT16*)__builtin_assume_aligned((SDs2->SiftDescriptorsBasicFeatures[DescriptorIndex2]), 16);
            __m128i m16;
            #define PARTIAL_SAD16(x) \
            m16 = LOAD_128_bits((__m128i*)(src2 + 8*x));\
            m16 =  _mm_sub_epi16(m16,m##x);\
            m16 = _mm_abs_epi16(m16);\
            _mm_store_si128((__m128i*)(multsU16 + 8*x), m16);

            PARTIAL_SAD16(0);PARTIAL_SAD16(1);PARTIAL_SAD16(2);PARTIAL_SAD16(3);PARTIAL_SAD16(4);PARTIAL_SAD16(5);PARTIAL_SAD16(6);PARTIAL_SAD16(7);
            PARTIAL_SAD16(8);PARTIAL_SAD16(9);PARTIAL_SAD16(10);PARTIAL_SAD16(11);PARTIAL_SAD16(12);PARTIAL_SAD16(13);PARTIAL_SAD16(14);PARTIAL_SAD16(15);

            for (int i = 0; i < 128; i++)
              dsq += multsU16[i];

            if (dsq < distsq1)
            {
                distsq2 = distsq1;
                distsq1 = dsq;
                //nexttominIndex = minIndex;
                minIndex = DescriptorIndex2;
            }
            else if (dsq < distsq2)
            {
                distsq2 = dsq;
                //nexttominIndex = DescriptorIndex2;
            }
       }

        if (distsq1 < (FACTOR1 * distsq2) >> FACTOR2)
            SMs->DescIx2ndImgMin[SMs->RealMatches++] = minIndex;
        //otherwise overwrite it next image1 descriptor
    }
}
#endif // __SSE__

#ifdef __ARM_NEON__
static void SAD_FindMatchesF32_SSE(SiftDescriptorsF32 *SDs1, SiftDescriptorsF32 *SDs2, SiftMatches* SMs) {};
#else
static void SAD_FindMatchesF32_SSE(SiftDescriptorsF32 *SDs1, SiftDescriptorsF32 *SDs2, SiftMatches* SMs)
{
    SMs->RealMatches = 0;
    int DescriptorIndex1;
    FLOAT32 multsF32[128] __attribute__ ((aligned(32)));

	for (DescriptorIndex1 =0; DescriptorIndex1 < SDs1->RealDescriptors; DescriptorIndex1++)
    {
		int DescriptorIndex2;
		int minIndex = 0;
		//int nexttominIndex;
		FLOAT32 dsq, distsq1, distsq2;
		distsq1 = (FLOAT32)UINT_MAX;
		distsq2 = (FLOAT32)UINT_MAX;

            FLOAT32 *src1 = (FLOAT32*)__builtin_assume_aligned((SDs1->SiftDescriptorsBasicFeatures[DescriptorIndex1]), 32);
            //load 128 Bytes of data (8 x (16x8) bits )

            #define LOAD_32F_SAD(y) __m256 m##y = LOAD_F32_256_bits(src1 + y*8);
            LOAD_32F_SAD(0);LOAD_32F_SAD(1);LOAD_32F_SAD(2);LOAD_32F_SAD(3);
            LOAD_32F_SAD(4);LOAD_32F_SAD(5);LOAD_32F_SAD(6);LOAD_32F_SAD(7);
            LOAD_32F_SAD(8);LOAD_32F_SAD(9);LOAD_32F_SAD(10);LOAD_32F_SAD(11);
            LOAD_32F_SAD(12);LOAD_32F_SAD(13);LOAD_32F_SAD(14);LOAD_32F_SAD(15);

        for (DescriptorIndex2 =0; DescriptorIndex2 < SDs2->RealDescriptors; DescriptorIndex2++)
	    {
	        dsq = 0;
            FLOAT32 *src2 = (FLOAT32*)__builtin_assume_aligned((SDs2->SiftDescriptorsBasicFeatures[DescriptorIndex2]), 32);
            __m256 m16, m17;

            #define PARTIAL_SAD_F32(x) \
            m16 = LOAD_F32_256_bits(src2 + x*8);\
            m17 = _mm256_sub_ps(m16,m##x);\
            m16 = _mm256_sub_ps(m##x, m16);\
            m16 = _mm256_max_ps (m16, m17);\
            m16 = _mm256_hadd_ps(m16,m16);\
             _mm256_store_ps(multsF32 + x*8, m16);

            PARTIAL_SAD_F32(0);            PARTIAL_SAD_F32(1);            PARTIAL_SAD_F32(2);            PARTIAL_SAD_F32(3);
            PARTIAL_SAD_F32(4);            PARTIAL_SAD_F32(5);            PARTIAL_SAD_F32(6);            PARTIAL_SAD_F32(7);
            PARTIAL_SAD_F32(8);            PARTIAL_SAD_F32(9);            PARTIAL_SAD_F32(10);           PARTIAL_SAD_F32(11);
            PARTIAL_SAD_F32(12);           PARTIAL_SAD_F32(13);           PARTIAL_SAD_F32(14);           PARTIAL_SAD_F32(15);

            for (int i = 0; i < 128; i+=4) dsq += multsF32[i] + multsF32[i+1];

            if (dsq < distsq1)
                {
                    distsq2 = distsq1;
                    distsq1 = dsq;
                    //nexttominIndex = minIndex;
                    minIndex = DescriptorIndex2;
                }
            else if (dsq < distsq2)
                {
                    distsq2 = dsq;
                    //nexttominIndex = DescriptorIndex2;
                }
        }
        if (distsq1 < (FACTOR1 * distsq2) / FACTOR2_VAL)
            SMs->DescIx2ndImgMin[SMs->RealMatches++] = minIndex;
        //otherwise overwrite it next image1 descriptor
    }
}
#endif // __SSE__


#ifdef __ARM_NEON__
//#include <arm_neon.h>
/*
 <type><size>x<number of lanes>_t
 <type><size>x<number of lanes>x<length of array>_t
struct uint16x4x2_t
{
   uint16x4_t val[2];
};
*/
int MainNeonSSE()
{
    //cout << "MainNeonSSE not implemented for arm "<<endl;
    UINT16 dataIn[] __attribute__ ((aligned(32))) = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
    uint16x4x4_t data __attribute__ ((aligned(32)))
    UINT16 dataOut[16] __attribute__ ((aligned(32)));

    //load 4 chuncks each having
    uint16x4x4_t data = vld4_u16(dataIn);

    //void vst4_u16 (uint16_t *, uint16x4x4_t)
    vst4_u16(dataOut, data);
    for (int i = 0; i < 16l i++)
        cout<<dataOut[i]<<" ";
    cout<<endl;
}
#else

int MainNeonSSE()
{
    FLOAT32 data1[] __attribute__ ((aligned(32))) = {0, 255,  0,   255, 0, 255,   255,255, 0,1,2,3,4,5,6,7};
    FLOAT32 data2[] __attribute__ ((aligned(32))) = {255, 0, 255,    0, 255, 0,   0   ,  0,7,6,5,4,3,2,1,0};
    FLOAT32 dataF32[128] __attribute__ ((aligned(32)));
    __m256 m16 = _mm256_load_ps(data1);
    __m256 m15 = _mm256_load_ps(data2);

    _mm256_store_ps(dataF32, m16);
    cout<<"DATA1= ";for (int i = 0; i < 8; i++)   cout<<dataF32[i]<<" ";cout<<endl;

    _mm256_store_ps(dataF32, m15);
    cout<<"DATA2= ";for (int i = 0; i < 8; i++)   cout<<dataF32[i]<<" ";cout<<endl;

    m16 = _mm256_sub_ps(m16,m15);
    _mm256_store_ps(dataF32, m16);
    cout<<"SUB= ";for (int i = 0; i < 8; i++)   cout<<dataF32[i]<<" ";cout<<endl;

    m16 = _mm256_mul_ps(m16,m16);
    _mm256_store_ps(dataF32, m16);
    cout<<"MULT= ";for (int i = 0; i < 8; i++)   cout<<dataF32[i]<<" ";cout<<endl;

    m16 = _mm256_hadd_ps(m16,m16);
    _mm256_store_ps(dataF32, m16);
    cout<<"HADD= ";for (int i = 0; i < 8; i++)   cout<<dataF32[i]<<" ";cout<<endl;

    FLOAT32 sum = 0;
    for (int i = 0; i < 8; i+=4)
    {
      sum += dataF32[i] + dataF32[i+1];
      cout<<dataF32[i]<<" ";
    }
    cout<<" SSD = "<<sum<<endl;

    sum =0;
    for (int i = 0; i < 8; i++)
        sum += (data1[i] - data2[i])*(data1[i] - data2[i]);
    cout<<" SSD = "<<sum<<endl;
    cin>>sum;
	return 0;

/*
    m16 = _mm256_sub_ps(m16,m##x);\
    m16 = _mm256_mul_ps(m16,m16);\
    m16 = _mm256_hadd_ps(m16,m16);\
     _mm256_store_ps(multsF32 + x*8, m16);

    PARTIAL_SSD_F32(0);            PARTIAL_SSD_F32(1);            PARTIAL_SSD_F32(2);            PARTIAL_SSD_F32(3);
    PARTIAL_SSD_F32(4);            PARTIAL_SSD_F32(5);            PARTIAL_SSD_F32(6);            PARTIAL_SSD_F32(7);
    PARTIAL_SSD_F32(8);            PARTIAL_SSD_F32(9);            PARTIAL_SSD_F32(10);           PARTIAL_SSD_F32(11);
    PARTIAL_SSD_F32(12);           PARTIAL_SSD_F32(13);           PARTIAL_SSD_F32(14);           PARTIAL_SSD_F32(15);

    for (int i = 0; i < 128; i+=4) dsq += multsF32[i] + multsF32[i+1];
        */
}

/*
int MainNeonSSE_Int()
{
    UINT8 dataU8[16] __attribute__ ((aligned16));
    UINT16 dataU16[8] __attribute__ ((aligned16));

    UINT32 Vendor[3];
    char VendorString[13];

    unsigned int maxFunc;
    cpuid(0, maxFunc, Vendor[0], Vendor[2], Vendor[1]);

    for (int i=0; i < 3; i++)
    {
        VendorString[4*i] = Vendor[i];
        VendorString[4*i + 1] = Vendor[i] >> 8;
        VendorString[4*i + 2] = Vendor[i] >> 16;
        VendorString[4*i + 3] = Vendor[i] >> 24;
    }
    VendorString[12]=0;

    cout<<maxFunc<<flush<<endl;
    cout<<VendorString<<flush<<endl;

            //m8 = _mm_load_si128((__m128i*)(src2+x*8));\
            //m8 = _mm_sad_epu8(m8,m##x);\
            //_mm_store_si128((__m128i*)(multsUI16+x*8), m8);

    UINT8 data1[] __attribute__ ((aligned16)) = {0, 255,  0,   255, 0, 255,   255,255, 0,1,2,3,4,5,6,7};
    UINT8 data2[] __attribute__ ((aligned16)) = {255, 0, 255,    0, 255, 0,   0   ,  0,7,6,5,4,3,2,1,0};

	__m128i m1 =  _mm_lddqu_si128((__m128i*)data1);
	__m128i m2 =  _mm_lddqu_si128((__m128i*)data2);
	__m128i m3, m4;

    _mm_store_si128((__m128i*)dataU8, m1);
    cout<<"M1= ";
    for (int i = 0; i < 16; i++)
        cout<<(UINT16)dataU8[i]<<" ";
    cout<<endl;

    _mm_store_si128((__m128i*)dataU8, m2);
    cout<<"M2= ";
    for (int i = 0; i < 16; i++)
        cout<<(UINT16)dataU8[i]<<" ";
    cout<<endl;


    m3 = _mm_sad_epu8(m1,m2);
    _mm_store_si128((__m128i*)dataU16, m3);
    cout<<"SAD = ";
    for (int i = 0; i < 8; i++)
        cout<<dataU16[i]<<" ";
    cout<<endl;

    UINT32 sum = 0;
    for (int i = 0; i < 8; i+=4)
    {
      sum += dataU16[i];
      cout<<dataU16[i]<<" ";
    }
    cout<<" SAD = "<<sum<<endl;

    sum =0;
    for (int i = 0; i < 16; i++)
        sum += abs(data1[i] - data2[i]);
    cout<<" SAD = "<<sum<<endl;
	return 0;
}
*/
#endif


static void SAD8_Benchmark(char* fileName1, char* fileName2)
{
    #ifdef __ARM_NEON__

    cout<<" SAD 8-bit not implemented with NEON " <<flush<<endl;

    #else
    /* 8-bit matching */
    int Start;

    cout<<"Starting SAD 8-bit... " <<flush<<endl;
    cout<<"---------------------- " <<flush<<endl;
    LoadDescriptors8(fileName1, &SiftDescriptors8_1, 0);
    LoadDescriptors8(fileName2, &SiftDescriptors8_2, 0);

    Start = GetMilliCount();
    SAD_FindMatches8(&SiftDescriptors8_1, &SiftDescriptors8_2, &SM_Arm);
    cout<<"CPU-only FindMatches ran in " << GetMilliSpan(Start)<< " ms"<<flush<<endl;

    Start = GetMilliCount();
    SAD_FindMatches8_SSE(&SiftDescriptors8_1, &SiftDescriptors8_2, &SM_Arm_SSE);
    cout<<"CPU-Only + SSE/NEON FindMatches ran in " << GetMilliSpan(Start)<< " ms"<<flush<<endl;

    if (PASS == CompareMatches(&SM_Arm,&SM_Arm_SSE)) cout << "Matches are a ... match ;). CPU and CPU-SSE/NEON got the same results."<<endl<<endl;
    else cout << "Match test has FAILed. CPU and CPU_SSE/NEON got different results !"<<endl<<endl;

    #endif // __ARM_NEON__

}

static void SAD16_Benchmark(char* fileName1, char* fileName2)
{
    #ifdef __ARM_NEON__

    cout<<" SAD 16-bit not implemented with NEON " <<flush<<endl;

    #else

        int Start;
        cout<<"Starting SAD 16-bit... " <<flush<<endl;
        cout<<"---------------------- " <<flush<<endl;
        LoadDescriptors16(fileName1, &SiftDescriptors16_1, 0);
        LoadDescriptors16(fileName2, &SiftDescriptors16_2, 0);

        Start = GetMilliCount();
        SAD_FindMatches16(&SiftDescriptors16_1, &SiftDescriptors16_2, &SM_Arm);
        cout<<"CPU-only FindMatches ran in " << GetMilliSpan(Start)<< " ms"<<flush<<endl;

        Start = GetMilliCount();
        SAD_FindMatches16_SSE(&SiftDescriptors16_1, &SiftDescriptors16_2, &SM_Arm_SSE);
        cout<<"CPU-Only + SSE/NEON FindMatches ran in " << GetMilliSpan(Start)<< " ms"<<flush<<endl;

        if (PASS == CompareMatches(&SM_Arm,&SM_Arm_SSE)) cout << "Matches are a ... match ;). CPU and CPU-SSE/NEON got the same results."<<endl<<endl;
        else cout << "Match test has FAILed. CPU and CPU_SSE/NEON got different results !"<<endl<<endl;
    #endif
}

static void SSD16_Benchmark(char* fileName1, char* fileName2)
{
    #ifdef __ARM_NEON__

    cout<<" SSD 16-bit not implemented with NEON " <<flush<<endl;

    #else

        int Start;
        cout<<"\n\nStarting SSD 16-bit... " <<flush<<endl;
        cout<<"---------------------- " <<flush<<endl;

        LoadDescriptors16(fileName1, &SiftDescriptors16_1, 0);
        LoadDescriptors16(fileName2, &SiftDescriptors16_2, 0);

        Start = GetMilliCount();
        SSD_FindMatches16(&SiftDescriptors16_1, &SiftDescriptors16_2, &SM_Arm);
        cout<<"CPU-only FindMatches ran in " << GetMilliSpan(Start)<< " ms"<<flush<<endl;

        Start = GetMilliCount();
        SSD_FindMatches16_SSE(&SiftDescriptors16_1, &SiftDescriptors16_2, &SM_Arm_SSE);
        cout<<"CPU-Only + SSE/NEON FindMatches ran in " << GetMilliSpan(Start)<< " ms"<<flush<<endl;

        if (PASS == CompareMatches(&SM_Arm,&SM_Arm_SSE)) cout << "Matches are a ... match ;). CPU and CPU-SSE/NEON got the same results."<<endl<<endl;
        else cout << "Match test has FAILed. CPU and CPU_SSE/NEON got different results !"<<endl<<endl;
    #endif
}

static void SAD32F_Benchmark(char* fileName1, char* fileName2)
{
    int Start;
    #ifdef __ARM_NEON__

    cout<<" SAD 8-bit not implemented with NEON " <<flush<<endl;

    #else

        cout<<"\n\nStarting SAD 32-bit float ... " <<flush<<endl;
        cout<<"---------------------- " <<flush<<endl;

        LoadDescriptorsF32(fileName1, &SiftDescriptorsF32_1, 0);
        LoadDescriptorsF32(fileName2, &SiftDescriptorsF32_2, 0);

        Start = GetMilliCount();
        SAD_FindMatchesF32(&SiftDescriptorsF32_1, &SiftDescriptorsF32_2, &SM_Arm);
        cout<<"CPU-only FindMatches ran in " << GetMilliSpan(Start)<< " ms"<<flush<<endl;

        Start = GetMilliCount();
        SAD_FindMatchesF32_SSE(&SiftDescriptorsF32_1, &SiftDescriptorsF32_2, &SM_Arm_SSE);
        cout<<"CPU-Only + SSE/NEON FindMatches ran in " << GetMilliSpan(Start)<< " ms"<<flush<<endl;

        if (PASS == CompareMatches(&SM_Arm,&SM_Arm_SSE)) cout << "Matches are a ... match ;). CPU and CPU-SSE/NEON got the same results."<<endl<<endl;
        else cout << "Match test has FAILed. CPU and CPU_SSE/NEON got different results !"<<endl<<endl;
    #endif
}

static void SSD32F_Benchmark(char* fileName1, char* fileName2)
{
    #ifdef __ARM_NEON__

        cout<<" SSD 32-bit not implemented with NEON " <<flush<<endl;

    #else
        int Start;

        cout<<"\n\nStarting SSD 32-bit float ... " <<flush<<endl;
        cout<<"---------------------- " <<flush<<endl;

        LoadDescriptorsF32(fileName1, &SiftDescriptorsF32_1, 0);
        LoadDescriptorsF32(fileName2, &SiftDescriptorsF32_2, 0);

        Start = GetMilliCount();
        SSD_FindMatchesF32(&SiftDescriptorsF32_1, &SiftDescriptorsF32_2, &SM_Arm);
        cout<<"CPU-only FindMatches ran in " << GetMilliSpan(Start)<< " ms"<<flush<<endl;

        Start = GetMilliCount();
        SSD_FindMatchesF32_SSE(&SiftDescriptorsF32_1, &SiftDescriptorsF32_2, &SM_Arm_SSE);
        cout<<"CPU-Only + SSE/NEON FindMatches ran in " << GetMilliSpan(Start)<< " ms"<<flush<<endl;

        if (PASS == CompareMatches(&SM_Arm,&SM_Arm_SSE)) cout << "Matches are a ... match ;). CPU and CPU-SSE/NEON got the same results."<<endl<<endl;
        else cout << "Match test has FAILed. CPU and CPU_SSE/NEON got different results !"<<endl<<endl;
    #endif
}

int test_BasicMatching_All_NeonSSE(char* fileName1, char* fileName2)
{
    int Start;

    SAD8_Benchmark(fileName1, fileName2);
    SAD16_Benchmark(fileName1, fileName2);
    SAD32F_Benchmark(fileName1, fileName2);

    SSD32F_Benchmark(fileName1, fileName2);
    SSD16_Benchmark(fileName1, fileName2);

	return 0;
}
