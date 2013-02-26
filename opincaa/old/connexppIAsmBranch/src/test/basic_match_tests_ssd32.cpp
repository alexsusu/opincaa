
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

#ifdef __ARM_NEON__
static void SSD_FindMatchesF32_NEON(SiftDescriptorsF32 *SDs1, SiftDescriptorsF32 *SDs2, SiftMatches* SMs) {};
#else
static void SSD_FindMatchesF32_SSE(SiftDescriptorsF32 *SDs1, SiftDescriptorsF32 *SDs2, SiftMatches* SMs)
{
    #define LOAD_F32_256_bits  _mm256_load_ps
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

void SSD32F_Benchmark(char* fileName1, char* fileName2)
{
    void (*SSD_FindMatchesF32_SSE_NEON) (SiftDescriptorsF32 *SDs1, SiftDescriptorsF32 *SDs2, SiftMatches* SMs);
    #ifdef __ARM_NEON__
        SSD_FindMatchesF32_SSE_NEON = SSD_FindMatchesF32_NEON;
    #else
        SSD_FindMatchesF32_SSE_NEON = SSD_FindMatchesF32_SSE;
    #endif // __ARM_NEON__

    AlingedSiftDescriptorPtrs SiftDescriptorsF32_1 = malloc_alligned(sizeof(SiftDescriptorsF32), 5);
    AlingedSiftDescriptorPtrs SiftDescriptorsF32_2 = malloc_alligned(sizeof(SiftDescriptorsF32), 5);
    SiftMatches *SM_Arm = (SiftMatches *)malloc(sizeof(SiftMatches));
    SiftMatches *SM_Arm_SSE_NEON = (SiftMatches *)malloc(sizeof(SiftMatches));

    if ((SiftDescriptorsF32_1.NonalignedValue == NULL) || (SM_Arm == NULL) || (SM_Arm_SSE_NEON == NULL))
    {
        cout<<" SSD32: Could not allocate memory"<<endl;
        return;
    }

    int Start;

    cout<<"\n\nStarting SSD 32-bit float ... " <<flush<<endl;
    cout<<"---------------------- " <<flush<<endl;

    LoadDescriptorsF32(fileName1, (SiftDescriptorsF32*)(SiftDescriptorsF32_1.AlignedValue), 0);
    LoadDescriptorsF32(fileName2, (SiftDescriptorsF32*)(SiftDescriptorsF32_2.AlignedValue), 0);

    Start = GetMilliCount();
    SSD_FindMatchesF32((SiftDescriptorsF32*)(SiftDescriptorsF32_1.AlignedValue),
                       (SiftDescriptorsF32*)(SiftDescriptorsF32_2.AlignedValue), SM_Arm);

    cout<<"CPU-only FindMatches ran in " << GetMilliSpan(Start)<< " ms"<<flush<<endl;

    Start = GetMilliCount();
    SSD_FindMatchesF32_SSE_NEON((SiftDescriptorsF32*)(SiftDescriptorsF32_1.AlignedValue),
                                (SiftDescriptorsF32*)(SiftDescriptorsF32_2.AlignedValue), SM_Arm_SSE_NEON);
    cout<<"CPU-Only + SSE/NEON FindMatches ran in " << GetMilliSpan(Start)<< " ms"<<flush<<endl;

    if (PASS == CompareMatches(SM_Arm, SM_Arm_SSE_NEON)) cout << "Matches are a ... match ;). CPU and CPU-SSE/NEON got the same results."<<endl<<endl;
    else cout << "Match test has FAILed. CPU and CPU_SSE/NEON got different results !"<<endl<<endl;

    free_alligned(SiftDescriptorsF32_1);
    free_alligned(SiftDescriptorsF32_2);
    free(SM_Arm);
    free(SM_Arm_SSE_NEON);
}

