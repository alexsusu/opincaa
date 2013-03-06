
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

#ifdef __ARM_NEON__
    #include <arm_neon.h>
#endif

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
static void print_int16x8(int16x8_t a)
{
        INT16 data[8];
        vst1q_s16(data, a);
        for (int i=0; i < 8; i++)
                cout<< data[i]<<" ";
        cout<<endl;
}

static void print_int16x8x4(int16x8x4_t a)
{
        INT16 data[32];
        vst4q_s16(data, a);
        for (int i=0; i < 32; i++)
                cout<< data[i]<<" ";
        cout<<endl;
}

static void print_int32x4x4(int32x4x4_t a)
{
        int  data[16];
        vst4q_s32(data, a);
        for (int i=0; i < 16; i++)
                cout<< data[i]<<" ";
        cout<<endl;
}

static void print_int64x2x4(int64x2x4_t a)
{
        long long int  data[8];

        vst1q_s64(&data[0], a.val[0]);
        vst1q_s64(&data[2], a.val[1]);
        vst1q_s64(&data[4], a.val[2]);
        vst1q_s64(&data[6], a.val[3]);

        for (int i=0; i < 8; i++)
                cout<< data[i]<<" ";
        cout<<endl;
}

static void SAD_Demo()
{
        int16x8x4_t num1;
        int16x8x4_t num2;
        int16x8x4_t calc;
        int32x4x4_t add1;
        int64x2x4_t add2;

        INT16 data1[] = {0,255,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31};
        INT16 data2[] = {255,0,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32};

        //INT16 datacalc[32];
        num1 = vld4q_s16(data1);
        num2 = vld4q_s16(data2);

        calc.val[0] = vsubq_s16(num1.val[0],num2.val[0]);
        calc.val[1] = vsubq_s16(num1.val[1],num2.val[1]);
        calc.val[2] = vsubq_s16(num1.val[2],num2.val[2]);
        calc.val[3] = vsubq_s16(num1.val[3],num2.val[3]);

        print_int16x8x4(num1);
        print_int16x8x4(num2);
        print_int16x8x4(calc);


        calc.val[0] = vabsq_s16(calc.val[0]);
        calc.val[1] = vabsq_s16(calc.val[1]);
        calc.val[2] = vabsq_s16(calc.val[2]);
        calc.val[3] = vabsq_s16(calc.val[3]);

        print_int16x8x4(calc);

        add1.val[0] = vpaddlq_s16(calc.val[0]);
        add1.val[1] = vpaddlq_s16(calc.val[1]);
        add1.val[2] = vpaddlq_s16(calc.val[2]);
        add1.val[3] = vpaddlq_s16(calc.val[3]);

        //add2.val[0] = vpaddlq_s32(add1.val[0]);
        //add2.val[1] = vpaddlq_s32(add1.val[1]);
        //add2.val[2] = vpaddlq_s32(add1.val[2]);
        //add2.val[3] = vpaddlq_s32(add1.val[3]);


}

static void SAD_FindMatches16_NEON(SiftDescriptors16 *SDs1, SiftDescriptors16 *SDs2, SiftMatches* SMs)
{
    SMs->RealMatches = 0;
    int minIndex = 0;
    //int nexttominIndex;
    UINT32 distsq1, distsq2;

    for (int DescriptorIndex1 =0; DescriptorIndex1 < SDs1->RealDescriptors; DescriptorIndex1++)
    {
        distsq1 = (UINT32)-1;
        distsq2 = (UINT32)-1;

            //load 128 bits as 8x 16 bits. Optimized for cache line of 64 Bytes = 512 bits (Intel SandyBridge)
            //INT16 *src1 = (INT16*)__builtin_assume_aligned((SDs1->SiftDescriptorsBasicFeatures[DescriptorIndex1]), 64);
            INT16 *src1 = (INT16*)(SDs1->SiftDescriptorsBasicFeatures[DescriptorIndex1]);

            int16x8x4_t desc1_0;
            int16x8x4_t desc1_1;
            int16x8x4_t desc1_2;
            int16x8x4_t desc1_3;

            //load 128 Bytes of data (4 x (16x8) bits )
            desc1_0 = vld4q_s16(src1);
            desc1_1 = vld4q_s16(src1 + 32);
            desc1_2 = vld4q_s16(src1 + 64);
            desc1_3 = vld4q_s16(src1 + 96);

        for (int DescriptorIndex2 =0; DescriptorIndex2 < SDs2->RealDescriptors; DescriptorIndex2++)
        {
            INT32 dsq = 0;
            INT16 multsI16[16*8] __attribute__ ((aligned(64)));

            //INT16 *src2 = (INT16*)__builtin_assume_aligned((SDs2->SiftDescriptorsBasicFeatures[DescriptorIndex2]), 64);
            INT16 *src2 = (INT16*)(SDs2->SiftDescriptorsBasicFeatures[DescriptorIndex2]);

            int16x8x4_t desc2_chunk;
            int16x8_t calc;

            #define PARTIAL_SAD16(x) \
            desc2_chunk = vld1q_s16(src2 + x*8);
            calc = vsubq_s16(desc1_##(x>>4).val[x & 3],desc2_chunk);\
            calc = vabsq_s16(calc);          \
            vst1q_s16(multsI16 + 8 * x, calc);

            PARTIAL_SAD16(0); PARTIAL_SAD16(1); PARTIAL_SAD16(2); PARTIAL_SAD16(3);
            PARTIAL_SAD16(4); PARTIAL_SAD16(5); PARTIAL_SAD16(6); PARTIAL_SAD16(7);
            PARTIAL_SAD16(8); PARTIAL_SAD16(9); PARTIAL_SAD16(10); PARTIAL_SAD16(11);
            PARTIAL_SAD16(12); PARTIAL_SAD16(13); PARTIAL_SAD16(14); PARTIAL_SAD16(15);

            for (int i = 0; i < 128; i++) dsq += multsI16[i];

            /*
            INT32 multsI32[16*4] __attribute__ ((aligned(64)));
            int16x8x4_t calc;
            int32x4x4_t addi32;

            #define PARTIAL_SAD16(x) \
            desc2_chunk = vld4q_s16(src2 + x * 32);\
            calc.val[0] = vsubq_s16(desc1_##x.val[0],desc2_chunk.val[0]);\
            calc.val[1] = vsubq_s16(desc1_##x.val[1],desc2_chunk.val[1]);\
            calc.val[2] = vsubq_s16(desc1_##x.val[2],desc2_chunk.val[2]);\
            calc.val[3] = vsubq_s16(desc1_##x.val[3],desc2_chunk.val[3]);\
                                                                    \
            calc.val[0] = vabsq_s16(calc.val[0]);                   \
            calc.val[1] = vabsq_s16(calc.val[1]);                   \
            calc.val[2] = vabsq_s16(calc.val[2]);                   \
            calc.val[3] = vabsq_s16(calc.val[3]);                   \
                                                                    \
            addi32.val[0] = vpaddlq_s16(calc.val[0]);               \
            addi32.val[1] = vpaddlq_s16(calc.val[1]);               \
            addi32.val[2] = vpaddlq_s16(calc.val[2]);               \
            addi32.val[3] = vpaddlq_s16(calc.val[3]);               \
                                                                    \
            vst4q_s32(multsI32 + 16 * x, addi32);

            PARTIAL_SAD16(0); PARTIAL_SAD16(1);  PARTIAL_SAD16(2); PARTIAL_SAD16(3);
            for (int i = 0; i < 64; i++) dsq += multsI32[i];

            */


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
            UINT16 *src1 = (UINT16*)__builtin_assume_aligned((SDs1->SiftDescriptorsBasicFeatures[DescriptorIndex1]), 64);
            //load 128 Bytes of data (8 x (16x8) bits )
            #define LOAD_128_bits_src1_16(x) __m128i m##x = LOAD_128_bits((__m128i*)(src1 + 8*x))
            LOAD_128_bits_src1_16(0);LOAD_128_bits_src1_16(1);LOAD_128_bits_src1_16(2);LOAD_128_bits_src1_16(3);
            LOAD_128_bits_src1_16(4);LOAD_128_bits_src1_16(5);LOAD_128_bits_src1_16(6);LOAD_128_bits_src1_16(7);
            LOAD_128_bits_src1_16(8);LOAD_128_bits_src1_16(9);LOAD_128_bits_src1_16(10);LOAD_128_bits_src1_16(11);
            LOAD_128_bits_src1_16(12);LOAD_128_bits_src1_16(13);LOAD_128_bits_src1_16(14);LOAD_128_bits_src1_16(15);

        for (int DescriptorIndex2 =0; DescriptorIndex2 < SDs2->RealDescriptors; DescriptorIndex2++)
	    {
	        dsq = 0;
            UINT16 *src2 = (UINT16*)__builtin_assume_aligned((SDs2->SiftDescriptorsBasicFeatures[DescriptorIndex2]), 64);
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
{
    SMs->RealMatches = 0;
    int *dsq = (int*) malloc(SDs1->RealDescriptors*SDs2->RealDescriptors*sizeof(int));
    if (dsq == NULL) {cout<<" Could not allocate memory "<<endl;return;}
    //    for (int er=0; er < SDs1->RealDescriptors; er++) dsq[er] = new int[SDs2->RealDescriptors];

        #pragma omp parallel for
        for (int DescriptorIndex1 =0; DescriptorIndex1 < SDs1->RealDescriptors; DescriptorIndex1++)
        {
            //load 128 bits as 8x 16 bits. Optimized for cache line of 64 Bytes = 512 bits (Intel SandyBridge)
            //INT16 *src1 = (INT16*)__builtin_assume_aligned((SDs1->SiftDescriptorsBasicFeatures[DescriptorIndex1]), 64);
            INT16 *src1 = (INT16*)(SDs1->SiftDescriptorsBasicFeatures[DescriptorIndex1]);

            int16x8x4_t desc1_0;
            int16x8x4_t desc1_1;
            int16x8x4_t desc1_2;
            int16x8x4_t desc1_3;

            //load 128 Bytes of data (4 x (16x8) bits )
            desc1_0 = vld4q_s16(src1);
            desc1_1 = vld4q_s16(src1 + 32);
            desc1_2 = vld4q_s16(src1 + 64);
            desc1_3 = vld4q_s16(src1 + 96);

            for (int DescriptorIndex2 =0; DescriptorIndex2 < SDs2->RealDescriptors; DescriptorIndex2++)
            {
                INT32 dsqs = 0;
                INT32 multsI32[16*4] __attribute__ ((aligned(64)));
                //INT16 *src2 = (INT16*)__builtin_assume_aligned((SDs2->SiftDescriptorsBasicFeatures[DescriptorIndex2]), 64);
                INT16 *src2 = (INT16*)(SDs2->SiftDescriptorsBasicFeatures[DescriptorIndex2]);

                int16x8x4_t desc2_chunk;
                int16x8x4_t calc;
                int32x4x4_t addi32;

                #define _PARTIAL_SAD16(x) \
                desc2_chunk = vld4q_s16(src2 + x * 32);\
                calc.val[0] = vsubq_s16(desc1_##x.val[0],desc2_chunk.val[0]);\
                calc.val[1] = vsubq_s16(desc1_##x.val[1],desc2_chunk.val[1]);\
                calc.val[2] = vsubq_s16(desc1_##x.val[2],desc2_chunk.val[2]);\
                calc.val[3] = vsubq_s16(desc1_##x.val[3],desc2_chunk.val[3]);\
                                                                        \
                calc.val[0] = vabsq_s16(calc.val[0]);                   \
                calc.val[1] = vabsq_s16(calc.val[1]);                   \
                calc.val[2] = vabsq_s16(calc.val[2]);                   \
                calc.val[3] = vabsq_s16(calc.val[3]);                   \
                                                                        \
                addi32.val[0] = vpaddlq_s16(calc.val[0]);               \
                addi32.val[1] = vpaddlq_s16(calc.val[1]);               \
                addi32.val[2] = vpaddlq_s16(calc.val[2]);               \
                addi32.val[3] = vpaddlq_s16(calc.val[3]);               \
                                                                        \
                vst4q_s32(multsI32 + 16 * x, addi32);

                _PARTIAL_SAD16(0);
                _PARTIAL_SAD16(1);
                _PARTIAL_SAD16(2);
                _PARTIAL_SAD16(3);

                for (int i = 0; i < 64; i++) dsqs += multsI32[i];
                dsq[DescriptorIndex1*SDs2->RealDescriptors+DescriptorIndex2] = dsqs;
        }

        for (int DescriptorIndex1 =0; DescriptorIndex1 < SDs1->RealDescriptors; DescriptorIndex1++)
        {
            int	minIndex = -1;
            unsigned int distsq1, distsq2;
            distsq1 = distsq2 = (unsigned int)(-1);
            for (int DescriptorIndex2 =0; DescriptorIndex2 < SDs2->RealDescriptors; DescriptorIndex2++)
            {
                if (dsq[DescriptorIndex1*SDs2->RealDescriptors+DescriptorIndex2] < distsq1)
                {
                    distsq2 = distsq1;
                    distsq1 = dsq[DescriptorIndex1*SDs2->RealDescriptors+DescriptorIndex2];
                    //nexttomin = imatch;
                    minIndex = DescriptorIndex2;
                }
                else if (dsq[DescriptorIndex1*SDs2->RealDescriptors+DescriptorIndex2] < distsq2)
                {
                    distsq2 = dsq[DescriptorIndex1*SDs2->RealDescriptors+DescriptorIndex2];
                    //nexttomin = j;
                }
            }
            if (distsq1 < (FACTOR1 * distsq2) >> FACTOR2)
                SMs->DescIx2ndImgMin[SMs->RealMatches++] = minIndex;
        }

//        for (int er=0; er < SDs1->RealDescriptors; er++) delete(dsq[er]);
        free(dsq);
}


}
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
            UINT16 *src1 = (UINT16*)__builtin_assume_aligned((SDs1->SiftDescriptorsBasicFeatures[DescriptorIndex1]), 64);
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
            UINT16 *src2 = (UINT16*)__builtin_assume_aligned((SDs2->SiftDescriptorsBasicFeatures[DescriptorIndex2]), 64);
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

    AlingedSiftDescriptorPtrs SiftDescriptors16_1 = malloc_alligned(sizeof(SiftDescriptors16), 6);
    AlingedSiftDescriptorPtrs SiftDescriptors16_2 = malloc_alligned(sizeof(SiftDescriptors16), 6);
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
