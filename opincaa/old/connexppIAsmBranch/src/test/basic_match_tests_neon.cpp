
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
#include <iostream>
#include <iomanip>

#include "../../include/test/basic_match_tests_commons.h"

#include "../../include/test/basic_match_tests_sad8.h"
#include "../../include/test/basic_match_tests_sad16.h"
#include "../../include/test/basic_match_tests_sad32.h"

#include "../../include/test/basic_match_tests_ssd16.h"
#include "../../include/test/basic_match_tests_ssd32.h"

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
    uint16x4x4_t data __attribute__ ((aligned(32)));
    UINT16 dataOut[16] __attribute__ ((aligned(32)));

    //load 4 chuncks each having
    data = vld4_u16(dataIn);

    //void vst4_u16 (uint16_t *, uint16x4x4_t)
    vst4_u16(dataOut, data);
    for (int i = 0; i < 16; i++)
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
