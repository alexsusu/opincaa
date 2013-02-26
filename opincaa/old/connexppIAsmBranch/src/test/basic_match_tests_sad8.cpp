
/*
 *
 * File: basic_match_tests_ssd16.cpp
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

#include "../../include/test/basic_match_tests_commons.h"
#include <iostream>
using namespace std;

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

#ifdef __ARM_NEON__
static void SAD_FindMatches8_NEON(SiftDescriptors8 *SDs1, SiftDescriptors8 *SDs2, SiftMatches* SMs)
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

void SAD8_Benchmark(char* fileName1, char* fileName2)
{
    void (*SAD_FindMatches8_SSE_NEON) (SiftDescriptors8 *SDs1, SiftDescriptors8 *SDs2, SiftMatches* SMs);
    #ifdef __ARM_NEON__
        SAD_FindMatches8_SSE_NEON = SAD_FindMatches8_NEON;
    #else
        SAD_FindMatches8_SSE_NEON = SAD_FindMatches8_SSE;
    #endif // __ARM_NEON__

    AlingedSiftDescriptorPtrs SiftDescriptors8_1 = malloc_alligned(sizeof(SiftDescriptors8), 5);
    AlingedSiftDescriptorPtrs SiftDescriptors8_2 = malloc_alligned(sizeof(SiftDescriptors8), 5);
    SiftMatches *SM_Arm = (SiftMatches *)malloc(sizeof(SiftMatches));
    SiftMatches *SM_Arm_SSE_NEON = (SiftMatches *)malloc(sizeof(SiftMatches));

    if ((SiftDescriptors8_1.NonalignedValue == NULL) || (SM_Arm == NULL) || (SM_Arm_SSE_NEON == NULL))
    {
        cout<<" Could not allocate memory"<<endl;
    }
    int Start;

    cout<<"Starting SAD 8-bit... " <<flush<<endl;
    cout<<"---------------------- " <<flush<<endl;
    LoadDescriptors8(fileName1, (SiftDescriptors8*)(SiftDescriptors8_1.AlignedValue), 0);
    LoadDescriptors8(fileName2, (SiftDescriptors8*)(SiftDescriptors8_2.AlignedValue), 0);

    Start = GetMilliCount();
    SAD_FindMatches8((SiftDescriptors8*)(SiftDescriptors8_1.AlignedValue), (SiftDescriptors8*)(SiftDescriptors8_2.AlignedValue), SM_Arm);
    cout<<"CPU-only FindMatches ran in " << GetMilliSpan(Start)<< " ms"<<flush<<endl;

    Start = GetMilliCount();
    SAD_FindMatches8_SSE_NEON((SiftDescriptors8*)(SiftDescriptors8_1.AlignedValue), (SiftDescriptors8*)(SiftDescriptors8_2.AlignedValue), SM_Arm_SSE_NEON);
    cout<<"CPU-Only + SSE/NEON FindMatches ran in " << GetMilliSpan(Start)<< " ms"<<flush<<endl;

    if (PASS == CompareMatches(SM_Arm,SM_Arm_SSE_NEON)) cout << "Matches are a ... match ;). CPU and CPU-SSE/NEON got the same results."<<endl<<endl;
    else cout << "Match test has FAILed. CPU and CPU_SSE/NEON got different results !"<<endl<<endl;

    free_alligned(SiftDescriptors8_1);
    free_alligned(SiftDescriptors8_2);
    free(SM_Arm);
    free(SM_Arm_SSE_NEON);
}
