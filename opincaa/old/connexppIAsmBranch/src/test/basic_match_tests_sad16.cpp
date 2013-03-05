
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

#ifdef __ARM_NEON__
static void SAD_FindMatches16_NEON(SiftDescriptors16 *SDs1, SiftDescriptors16 *SDs2, SiftMatches* SMs)
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
static void SAD_FindMatches16_OMP_NEON(SiftDescriptors16 *SDs1, SiftDescriptors16 *SDs2, SiftMatches* SMs)
{}
#else
static void SAD_FindMatches16_OMP_SSE(SiftDescriptors16 *SDs1, SiftDescriptors16 *SDs2, SiftMatches* SMs)
{
    SMs->RealMatches = 0;

    int **dsq = new int*[SDs1->RealDescriptors];
    for (int er=0; er < SDs1->RealDescriptors; er++) dsq[er] = new int[SDs2->RealDescriptors];

    #pragma omp parallel for
    for (int DescriptorIndex1 =0; DescriptorIndex1 < SDs1->RealDescriptors; DescriptorIndex1++)
    {
            //load 128 bits as 8x 16 bits. Optimized for cache line of 64 Bytes = 512 bits (Intel SandyBridge)
            UINT16 *src1 = (UINT16*)__builtin_assume_aligned((SDs1->SiftDescriptorsBasicFeatures[DescriptorIndex1]), 16);
            //load 128 Bytes of data (8 x (16x8) bits )
            #define LOAD_128_bits_src1_16(x) __m128i m##x = LOAD_128_bits((__m128i*)(src1 + 8*x))
            LOAD_128_bits_src1_16(0);LOAD_128_bits_src1_16(1);LOAD_128_bits_src1_16(2);LOAD_128_bits_src1_16(3);
            LOAD_128_bits_src1_16(4);LOAD_128_bits_src1_16(5);LOAD_128_bits_src1_16(6);LOAD_128_bits_src1_16(7);
            LOAD_128_bits_src1_16(8);LOAD_128_bits_src1_16(9);LOAD_128_bits_src1_16(10);LOAD_128_bits_src1_16(11);
            LOAD_128_bits_src1_16(12);LOAD_128_bits_src1_16(13);LOAD_128_bits_src1_16(14);LOAD_128_bits_src1_16(15);

        #pragma omp parallel for
        for (int DescriptorIndex2 =0; DescriptorIndex2 < SDs2->RealDescriptors; DescriptorIndex2++)
	    {

	        UINT32 dsqs = 0;
	        UINT16 multsU16[8*16] __attribute__ ((aligned(16)));
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
              dsqs += multsU16[i];

            dsq[DescriptorIndex1][DescriptorIndex2] = dsqs;
            //dsq[DescriptorIndex1][DescriptorIndex2] = SAD16_Distance()
	    }
    }

    for (int DescriptorIndex1 =0; DescriptorIndex1 < SDs1->RealDescriptors; DescriptorIndex1++)
        {
            int	minIndex = -1;
            unsigned int distsq1, distsq2;
            distsq1 = distsq2 = (unsigned int)(-1);
            for (int DescriptorIndex2 =0; DescriptorIndex2 < SDs2->RealDescriptors; DescriptorIndex2++)
            {
                if (dsq[DescriptorIndex1][DescriptorIndex2] < distsq1)
                {
                    distsq2 = distsq1;
                    distsq1 = dsq[DescriptorIndex1][DescriptorIndex2];
                    //nexttomin = imatch;
                    minIndex = DescriptorIndex2;
                }
                else if (dsq[DescriptorIndex1][DescriptorIndex2] < distsq2)
                {
                    distsq2 = dsq[DescriptorIndex1][DescriptorIndex2];
                    //nexttomin = j;
                }
            }
            if (distsq1 < (FACTOR1 * distsq2) >> FACTOR2)
                SMs->DescIx2ndImgMin[SMs->RealMatches++] = minIndex;
        }

        for (int er=0; er < SDs1->RealDescriptors; er++) delete(dsq[er]);
        delete(dsq);
}
#endif // __SSE__

void SAD16_Benchmark(char* fileName1, char* fileName2, FILE* logfile)
{
    void (*SAD_FindMatches16_SSE_NEON) (SiftDescriptors16 *SDs1, SiftDescriptors16 *SDs2, SiftMatches* SMs);
    void (*SAD_FindMatches16_OMP_SSE_NEON) (SiftDescriptors16 *SDs1, SiftDescriptors16 *SDs2, SiftMatches* SMs);
    #ifdef __ARM_NEON__
        SAD_FindMatches16_SSE_NEON = SAD_FindMatches16_NEON;
        SAD_FindMatches16_OMP_SSE_NEON = SAD_FindMatches16_OMP_NEON;
    #else
        SAD_FindMatches16_SSE_NEON = SAD_FindMatches16_SSE;
        SAD_FindMatches16_OMP_SSE_NEON = SAD_FindMatches16_OMP_SSE;
    #endif // __ARM_NEON__

    AlingedSiftDescriptorPtrs SiftDescriptors16_1 = malloc_alligned(sizeof(SiftDescriptors16), 5);
    AlingedSiftDescriptorPtrs SiftDescriptors16_2 = malloc_alligned(sizeof(SiftDescriptors16), 5);
    SiftMatches *SM_Cpu = (SiftMatches *)malloc(sizeof(SiftMatches));
    SiftMatches *SM_Cpu_SSE_NEON = (SiftMatches *)malloc(sizeof(SiftMatches));
    SiftMatches *SM_Cpu_OMP_SSE_NEON = (SiftMatches *)malloc(sizeof(SiftMatches));

    if ((SiftDescriptors16_1.NonalignedValue == NULL) || (SM_Cpu == NULL) || (SM_Cpu_SSE_NEON == NULL))
    {
        cout<<" SAD16: Could not allocate memory"<<endl;
        return;
    }

    int Start, Delta;
    cout<<"Starting SAD 16-bit... " <<flush<<endl;
    fprintf(logfile, "\nStarting SAD 16-bit...\n");

    cout<<"---------------------- " <<flush<<endl;

    LoadDescriptors16(fileName1, (SiftDescriptors16*)(SiftDescriptors16_1.AlignedValue), 0);
    LoadDescriptors16(fileName2, (SiftDescriptors16*)(SiftDescriptors16_2.AlignedValue), 0);
    float BruteMatches = ((SiftDescriptors16*)(SiftDescriptors16_1.AlignedValue))->RealDescriptors *
                            ((SiftDescriptors16*)(SiftDescriptors16_2.AlignedValue))->RealDescriptors;

    Start = GetMilliCount();
    SAD_FindMatches16((SiftDescriptors16*)(SiftDescriptors16_1.AlignedValue), (SiftDescriptors16*)(SiftDescriptors16_2.AlignedValue), SM_Cpu);
    Delta  = GetMilliSpan(Start);
    cout<<"> CPU FindMatches ran in " << Delta << " ms ("<< BruteMatches/Delta/1000 <<" MM/s)"<<flush<<endl;
    fprintf(logfile, "CPU_FindMatches_ran_in_time %d %f MM/s \n", Delta, BruteMatches/Delta/1000);

    Start = GetMilliCount();
    SAD_FindMatches16_SSE_NEON((SiftDescriptors16*)(SiftDescriptors16_1.AlignedValue),
                                (SiftDescriptors16*)(SiftDescriptors16_2.AlignedValue), SM_Cpu_SSE_NEON);
    Delta  = GetMilliSpan(Start);
    cout<<"> CPU + SSE/NEON FindMatches ran in " << Delta << " ms ("<< BruteMatches/Delta/1000 <<" MM/s)"<<flush<<endl;
    fprintf(logfile, "CPU_SSE/NEON_FindMatches_ran_in_time %d %f MM/s \n", Delta, BruteMatches/Delta/1000);

    Start = GetMilliCount();
    SAD_FindMatches16_OMP_SSE_NEON((SiftDescriptors16*)(SiftDescriptors16_1.AlignedValue),
                                (SiftDescriptors16*)(SiftDescriptors16_2.AlignedValue), SM_Cpu_OMP_SSE_NEON);
    cout<<"" << GetMilliSpan(Start)<< " ms"<<flush<<endl;
    Delta  = GetMilliSpan(Start);
    cout<<"> CPU + OPENMP + SSE/NEON FindMatches ran in  " << Delta << " ms ("<< BruteMatches/Delta/1000 <<" MM/s)"<<flush<<endl;
    fprintf(logfile, "CPU+OPENMP+SSE/NEON_FindMatches_ran_in_time %d %f MM/s \n", Delta, BruteMatches/Delta/1000);


    if (PASS == CompareMatches(SM_Cpu, SM_Cpu_OMP_SSE_NEON))
        cout << "Matches are a ... match ;). CPU and CPU_OMP-SSE/NEON got the same results."<<endl<<endl;
    else
        cout << "Match test has FAILed. CPU and CPU_SSE/NEON got different results !"<<endl<<endl;

    free_alligned(SiftDescriptors16_1);
    free_alligned(SiftDescriptors16_2);
    free(SM_Cpu);
    free(SM_Cpu_SSE_NEON);
    free(SM_Cpu_OMP_SSE_NEON);
}
