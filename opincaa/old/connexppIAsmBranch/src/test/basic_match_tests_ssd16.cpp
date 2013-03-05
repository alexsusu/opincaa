
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

//#define __ARM_NEON__
#ifdef __ARM_NEON__

/*
 <type><size>x<number of lanes>_t
 <type><size>x<number of lanes>x<length of array>_t
struct uint16x4x2_t
{
   uint16x4_t val[2];
};
*/
/*
{
    //cout << "MainNeonSSE not implemented for arm "<<endl;
    UINT16 dataIn[] __attribute__ ((aligned(32))) = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
    uint16x4x4_t data __attribute__ ((aligned(32)));
    UINT16 dataOut[16] __attribute__ ((aligned(32)));

    //load 4 chuncks each having
    data = vld4_u16(dataIn);

    //void vst4_u16 (uint16_t *, uint16x4x4_t)
    vst4_u16(dataOut, data);
}
*/
static void SSD_FindMatches16_NEON(SiftDescriptors16 *SDs1, SiftDescriptors16 *SDs2, SiftMatches* SMs)
{
    SMs->RealMatches = 0;
    int DescriptorIndex1;
    UINT16 multsUI16[8*16] __attribute__ ((aligned(32)));

	for (DescriptorIndex1 =0; DescriptorIndex1 < SDs1->RealDescriptors; DescriptorIndex1++)
    {
		int DescriptorIndex2;
		int minIndex = 0;
		//int nexttominIndex;
		UINT32 dsq, distsq1, distsq2;
		distsq1 = (UINT32)-1;
		distsq2 = (UINT32)-1;

            //load 128 bits as 8x 16 bits. Optimized for cache line of 64 Bytes = 512 bits (Intel SandyBridge)
            UINT16 *src1 = (UINT16*)__builtin_assume_aligned((SDs1->SiftDescriptorsBasicFeatures[DescriptorIndex1]), 32);
            //load 128 Bytes of data (8 x (16x8) bits )
            #define LOAD_512_bits(x) vld4q_u16(x)

            //uint16x8_t  vld1q_u16(__transfersize(8) uint16_t const * ptr); // VLD1.16 {d0, d1}, [r0]
            #define LOAD_128_bits(x) vld1q_u16(x)
            #define LOAD_SSD_FindMatches16_NEON(x) uint16x8x4_t data_##x __attribute__ ((aligned(32))) = LOAD_512_bits(src1 + 32 * x);
            LOAD_SSD_FindMatches16_NEON(0);
            LOAD_SSD_FindMatches16_NEON(1);
            LOAD_SSD_FindMatches16_NEON(2);
            LOAD_SSD_FindMatches16_NEON(3);

        for (DescriptorIndex2 =0; DescriptorIndex2 < SDs2->RealDescriptors; DescriptorIndex2++)
	    {
	        dsq = 0;
            UINT16 *src2 = (UINT16*)__builtin_assume_aligned((SDs2->SiftDescriptorsBasicFeatures[DescriptorIndex2]), 16);
            // uint16x8_t  vsubq_u16(uint16x8_t a, uint16x8_t b);   // VSUB.I16 q0,q0,q0
            // uint16x8_t  vmulq_u16(uint16x8_t a, uint16x8_t b);   // VMUL.I16 q0,q0,q0
            // void  vst1q_u16(__transfersize(8) uint16_t * ptr, uint16x8_t val); // VST1.16 {d0, d1}, [r0]

            uint16x8_t data_4;
            #define PARTIAL_SSD_NEON(x,y, z) \
            data_4 = LOAD_128_bits(src2+x*8);\
            data_4 = vsubq_u16(data_##y.val[z], data_4);\
            data_4 = vmulq_u16(data_4, data_4);\
            vst1q_u16(multsUI16 + x*8, data_4);

            PARTIAL_SSD_NEON(0,0,0);           PARTIAL_SSD_NEON(1,0,1);            PARTIAL_SSD_NEON(2,0,2);         PARTIAL_SSD_NEON(3,0,3);
            PARTIAL_SSD_NEON(4,1,0);           PARTIAL_SSD_NEON(5,1,1);            PARTIAL_SSD_NEON(6,1,2);         PARTIAL_SSD_NEON(7,1,3);
            PARTIAL_SSD_NEON(8,2,0);           PARTIAL_SSD_NEON(9,2,1);            PARTIAL_SSD_NEON(10,2,2);        PARTIAL_SSD_NEON(11,2,3);
            PARTIAL_SSD_NEON(12,3,0);          PARTIAL_SSD_NEON(13,3,1);           PARTIAL_SSD_NEON(14,3,2);        PARTIAL_SSD_NEON(15,3,3);

            for (int i = 0; i < 128; i++)
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
#else

/*
    It is pushing the limit of the SSE register count.
    It uses 17 registers out of the 16 existing.
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
static void SSD_FindMatches16_OMP_NEON(SiftDescriptors16 *SDs1, SiftDescriptors16 *SDs2, SiftMatches* SMs)
{}
#else
static void SSD_FindMatches16_OMP_SSE(SiftDescriptors16 *SDs1, SiftDescriptors16 *SDs2, SiftMatches* SMs)
{
    SMs->RealMatches = 0;

    int **dsq = new int*[SDs1->RealDescriptors];
    for (int er=0; er < SDs1->RealDescriptors; er++) dsq[er] = new int[SDs2->RealDescriptors];

    #pragma omp parallel for
    for (int DescriptorIndex1 =0; DescriptorIndex1 < SDs1->RealDescriptors; DescriptorIndex1++)
    {
            //load 128 bits as 8x 16 bits. Optimized for cache line of 64 Bytes = 512 bits (Intel SandyBridge)
            INT16 *src1 = (INT16*)__builtin_assume_aligned((SDs1->SiftDescriptorsBasicFeatures[DescriptorIndex1]), 16);
            //load 128 Bytes of data (8 x (16x8) bits )
            #define LOAD_SSD_FindMatches16_SSE(x) __m128i m##x = LOAD_128_bits((__m128i*)(src1 + 8 * x));
            LOAD_SSD_FindMatches16_SSE(0);LOAD_SSD_FindMatches16_SSE(1);LOAD_SSD_FindMatches16_SSE(2);LOAD_SSD_FindMatches16_SSE(3);
            LOAD_SSD_FindMatches16_SSE(4);LOAD_SSD_FindMatches16_SSE(5);LOAD_SSD_FindMatches16_SSE(6);LOAD_SSD_FindMatches16_SSE(7);
            LOAD_SSD_FindMatches16_SSE(8);LOAD_SSD_FindMatches16_SSE(9);LOAD_SSD_FindMatches16_SSE(10);LOAD_SSD_FindMatches16_SSE(11);
            LOAD_SSD_FindMatches16_SSE(12);LOAD_SSD_FindMatches16_SSE(13);LOAD_SSD_FindMatches16_SSE(14);LOAD_SSD_FindMatches16_SSE(15);

        #pragma omp parallel for
        for (int DescriptorIndex2 =0; DescriptorIndex2 < SDs2->RealDescriptors; DescriptorIndex2++)
	    {

            int dsqs = 0;
            INT32 multsI32[4*16] __attribute__ ((aligned(16)));
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
                dsqs += multsI32[i];

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

void SSD16_Benchmark(char* fileName1, char* fileName2, FILE* logfile)
{
    void (*SSD_FindMatches16_SSE_NEON) (SiftDescriptors16 *SDs1, SiftDescriptors16 *SDs2, SiftMatches* SMs);
    void (*SSD_FindMatches16_OMP_SSE_NEON) (SiftDescriptors16 *SDs1, SiftDescriptors16 *SDs2, SiftMatches* SMs);
    #ifdef __ARM_NEON__
        SSD_FindMatches16_SSE_NEON = SSD_FindMatches16_NEON;
        SSD_FindMatches16_OMP_SSE_NEON = SSD_FindMatches16_OMP_NEON;
    #else
        SSD_FindMatches16_SSE_NEON = SSD_FindMatches16_SSE;
        SSD_FindMatches16_OMP_SSE_NEON = SSD_FindMatches16_OMP_SSE;
    #endif // __ARM_NEON__

    AlingedSiftDescriptorPtrs SiftDescriptors16_1 = malloc_alligned(sizeof(SiftDescriptors16), 5);
    AlingedSiftDescriptorPtrs SiftDescriptors16_2 = malloc_alligned(sizeof(SiftDescriptors16), 5);
    SiftMatches *SM_Cpu = (SiftMatches *)malloc(sizeof(SiftMatches));
    SiftMatches *SM_Cpu_SSE_NEON = (SiftMatches *)malloc(sizeof(SiftMatches));
    SiftMatches *SM_Cpu_OMP_SSE_NEON = (SiftMatches *)malloc(sizeof(SiftMatches));

    if ((SiftDescriptors16_1.NonalignedValue == NULL) || (SM_Cpu == NULL) || (SM_Cpu_SSE_NEON == NULL))
    {
        cout<<" SSD16: Could not allocate memory"<<endl;
        return;
    }

    int Start, Delta;
    cout<<"Starting SSD 16-bit... " <<flush<<endl;
    fprintf(logfile, "\nStarting SSD 16-bit...\n");

    cout<<"---------------------- " <<flush<<endl;

    LoadDescriptors16(fileName1, (SiftDescriptors16*)(SiftDescriptors16_1.AlignedValue), 0);
    LoadDescriptors16(fileName2, (SiftDescriptors16*)(SiftDescriptors16_2.AlignedValue), 0);
    float BruteMatches = ((SiftDescriptors16*)(SiftDescriptors16_1.AlignedValue))->RealDescriptors *
                            ((SiftDescriptors16*)(SiftDescriptors16_2.AlignedValue))->RealDescriptors;



    Start = GetMilliCount();
    SSD_FindMatches16((SiftDescriptors16*)(SiftDescriptors16_1.AlignedValue), (SiftDescriptors16*)(SiftDescriptors16_2.AlignedValue), SM_Cpu);
    Delta  = GetMilliSpan(Start);
    cout<<"> CPU FindMatches ran in " << Delta << " ms ("<< BruteMatches/Delta/1000 <<" MM/s)"<<flush<<endl;
    fprintf(logfile, "CPU_FindMatches_ran_in_time %d %f MM/s \n", Delta, BruteMatches/Delta/1000);

    Start = GetMilliCount();
    SSD_FindMatches16_SSE_NEON((SiftDescriptors16*)(SiftDescriptors16_1.AlignedValue),
                                (SiftDescriptors16*)(SiftDescriptors16_2.AlignedValue), SM_Cpu_SSE_NEON);
    Delta  = GetMilliSpan(Start);
    cout<<"> CPU + SSE/NEON FindMatches ran in " << Delta << " ms ("<< BruteMatches/Delta/1000 <<" MM/s)"<<flush<<endl;
    fprintf(logfile, "CPU_SSE/NEON_FindMatches_ran_in_time %d %f MM/s \n", Delta, BruteMatches/Delta/1000);

    Start = GetMilliCount();
    SSD_FindMatches16_OMP_SSE_NEON((SiftDescriptors16*)(SiftDescriptors16_1.AlignedValue),
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

