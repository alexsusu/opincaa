
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

struct SiftDescriptors
{
    UINT16 SiftDescriptorsBasicFeatures[MAX_DESCRIPTORS][FEATURES_PER_DESCRIPTOR];
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

static SiftDescriptors SiftDescriptors1 __declspec(align(32));
static SiftDescriptors SiftDescriptors2 __declspec(align(32));
static SiftMatches SM_Arm;
static SiftMatches SM_Arm_SSE;
static SiftMatches SM_ConnexArm;
static SiftMatches SM_ConnexArmMan;
static UINT_RED_REG_VAL BasicMatchRedResults[MAX_REDUCES];

static void PrintDescriptors(SiftDescriptors *SDs)
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
        printf("Matches[%d].DescriptorIndexInSecondImage = %d \n", SMs->DescIx2ndImgMin[MatchesIndex]);
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

static int LoadDescriptors(char *FileName, SiftDescriptors *SDs, int Limit)
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
                result = fscanf(fp,"%d",&SDs->SiftDescriptorsBasicFeatures[DescriptorIndex][FeatIndex]);

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
static void FindMatches(SiftDescriptors *SDs1, SiftDescriptors *SDs2, SiftMatches* SMs)
{
    SMs->RealMatches = 0;
    int DescriptorIndex1;
    /*
    if (SDs1->RealDescriptors > SDs2->RealDescriptors)
    {
        SiftDescriptors *SDsman = SDs1;
        SDs1 = SDs2;
        SDs2 = SDsman;
    }
    */

	for (DescriptorIndex1 =0; DescriptorIndex1 < SDs1->RealDescriptors; DescriptorIndex1++)
    {
		int DescriptorIndex2;
		int minIndex;
		int nexttominIndex;
		UINT32 dsq, distsq1, distsq2;
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

/*
    Out of 1024 LocalStore locations, each having 128 Bytes:
    (fortunately a SIFT descriptor also has 128 features of 1 Bytes each)
*/
#define VECTORS_CHUNK_IMAGE1 364  //  364 cnxvectors - reserved for img1 (cnxvectorS_CHUNK_IMG1)
#define VECTORS_CHUNK_IMAGE2 330 // 330 cnxvectors - reserved for img2 - work (cnxvectorS_CHUNK_IMG2)
//another 330 cnxvectors - reserved for img2 - transfer with IO

#define VECTORS_SUBCHUNK_IMAGE2 30 // 30 cnxvectors from VECTORS_CHUNK_IMAGE2 to be cached for inner loop of distance calculation

static io_unit IOU_CVCI1;
static io_unit IOU_CVCI2;
#define MODE_CREATE_BATCHES 0
#define MODE_EXECUTE_FIND_MATCHES 1

static int connexFM_CreateBatch(int LoadToRxBatchNumber, int UsingBuffer0or1)
{
    int TotalcnxvectorSubChunksImg2 = VECTORS_CHUNK_IMAGE2 / VECTORS_SUBCHUNK_IMAGE2;
    BEGIN_BATCH(LoadToRxBatchNumber);
        //forall subchunks of chunk of img 2
        for(int CurrentcnxvectorSubChunkImg2 = 0; CurrentcnxvectorSubChunkImg2 < TotalcnxvectorSubChunksImg2; CurrentcnxvectorSubChunkImg2++)
        {
            //forall cnxvectors in subchunk of chunk of img (~30 cnxvectors) load cnxvector x to Rx
            for(int x = 0; x < 30; x++)
                R[x] = LS[VECTORS_CHUNK_IMAGE1 + UsingBuffer0or1*VECTORS_CHUNK_IMAGE2 +
                            CurrentcnxvectorSubChunkImg2 * VECTORS_SUBCHUNK_IMAGE2 + x];
            //forall 364 cnxvectors "y" in chunk of image 1
            for (int y = 0; y < VECTORS_CHUNK_IMAGE1; y++)
            {
                R[30] = LS[y]; //load cnxvector y to R30 ; cout <<" LS[" <<y<<"] ====== "<<endl;
                //forall registers with cnxvector-subchunk of img 2 (~30 cnxvectors in 30 registers)
                for(int x = 0; x < 30; x++)
                {
                    R31 = R30 - R[x];
                    R31 = R31 * R31;
                    REDUCE(R31);
                }
            }
        }
    END_BATCH();
    return PASS;
}

static int connexFindMatchesPass1(int RunningMode,int LoadToRxBatchNumber,
                                    SiftDescriptors *SiftDescriptors1, SiftDescriptors *SiftDescriptors2)
{
    int CurrentcnxvectorChunkImg1, CurrentcnxvectorChunkImg2;
    int TotalcnxvectorChunksImg1 = (SiftDescriptors1->RealDescriptors + VECTORS_CHUNK_IMAGE1 - 1) / VECTORS_CHUNK_IMAGE1;
    int TotalcnxvectorChunksImg2 = (SiftDescriptors2->RealDescriptors + VECTORS_CHUNK_IMAGE2 - 1) / VECTORS_CHUNK_IMAGE2;
    int TotalcnxvectorSubChunksImg2 = VECTORS_CHUNK_IMAGE2 / VECTORS_SUBCHUNK_IMAGE2;
    int UsingBuffer0or1;
    int TimeStart;
    int TotalIOTime = 0, TotalBatchTime = 0, TotalReductionTime = 0;

    //forall cnxvector chunks in img1
    for(CurrentcnxvectorChunkImg1 = 0; CurrentcnxvectorChunkImg1 < TotalcnxvectorChunksImg1; CurrentcnxvectorChunkImg1++)
    {
        UsingBuffer0or1 = 0;
        //>>>>IO-load cnxvector chunk on img1 to LocalStore[0...363]
        if (RunningMode == MODE_EXECUTE_FIND_MATCHES)
        {
            TimeStart = GetMilliCount();
            IOU_CVCI1.preWritecnxvectors(0,SiftDescriptors1->SiftDescriptorsBasicFeatures[VECTORS_CHUNK_IMAGE1*CurrentcnxvectorChunkImg1],VECTORS_CHUNK_IMAGE1);
            if (PASS != IO_WRITE_NOW(&IOU_CVCI1)) {   printf("Writing next CurrentcnxvectorChunkImg1 to IO pipe, FAILED !"); return FAIL;}
            TotalIOTime += GetMilliSpan(TimeStart);
        }

        //forall cnxvector chunks in img2
        for(CurrentcnxvectorChunkImg2 = 0; CurrentcnxvectorChunkImg2 < TotalcnxvectorChunksImg2; CurrentcnxvectorChunkImg2++)
        {
            UsingBuffer0or1 = CurrentcnxvectorChunkImg2 & 0x01;
            if (RunningMode == MODE_EXECUTE_FIND_MATCHES)
            {
                //>>>> BLOCKING_IO-load cnxvector chunk on img2 to LocalStore[364 ... 364 + 329] or [364+329 ... 1023] (aka wait for loading;)
                //if still have data, start NON_BLOCKING_IO-load cnxvector chunk on img2 to LS[364+329 ... 1023] or [364 ... 364 + 329]
                    TimeStart = GetMilliCount();
                    IOU_CVCI2.preWritecnxvectors(VECTORS_CHUNK_IMAGE1 + UsingBuffer0or1*VECTORS_CHUNK_IMAGE2,
                                                    SiftDescriptors2->SiftDescriptorsBasicFeatures[VECTORS_CHUNK_IMAGE2*CurrentcnxvectorChunkImg2],
                                                        VECTORS_CHUNK_IMAGE2);
                    TotalIOTime += GetMilliSpan(TimeStart);
                    if (PASS != IO_WRITE_NOW(&IOU_CVCI2))
                    {
                        printf("Writing next CurrentcnxvectorChunkImg2 to IO pipe, FAILED !");
                        return FAIL;
                    }
                TimeStart = GetMilliCount();
                EXECUTE_BATCH(LoadToRxBatchNumber + UsingBuffer0or1);
                TotalBatchTime += GetMilliSpan(TimeStart);

                {
                    int ExpectedBytesOfReductions = BYTES_IN_DWORD* VECTORS_CHUNK_IMAGE1 * VECTORS_CHUNK_IMAGE2;
                    TimeStart = GetMilliCount();
                    int RealBytesOfReductions = GET_MULTIRED_RESULT(BasicMatchRedResults +
                                        VECTORS_CHUNK_IMAGE1 * (TotalcnxvectorChunksImg2*VECTORS_CHUNK_IMAGE2) * CurrentcnxvectorChunkImg1 +
                                        VECTORS_CHUNK_IMAGE1 * VECTORS_CHUNK_IMAGE2 * CurrentcnxvectorChunkImg2,
                                        ExpectedBytesOfReductions
                                        );
                    TotalReductionTime += GetMilliSpan(TimeStart);
                    if (ExpectedBytesOfReductions != RealBytesOfReductions)
                     cout<<" Unexpected size of bytes of reductions (expected: "<<ExpectedBytesOfReductions<<" but got "<<RealBytesOfReductions<<endl;
                }
            }
            //next: create or execute created batch
            else// (RunningMode == MODE_CREATE_BATCHES)
            {
                BEGIN_BATCH(LoadToRxBatchNumber + UsingBuffer0or1);
                    //forall subchunks of chunk of img 2
                    for(int CurrentcnxvectorSubChunkImg2 = 0; CurrentcnxvectorSubChunkImg2 < TotalcnxvectorSubChunksImg2; CurrentcnxvectorSubChunkImg2++)
                    {
                        //forall cnxvectors in subchunk of chunk of img (~30 cnxvectors) load cnxvector x to Rx
                        for(int x = 0; x < 30; x++)
                            R[x] = LS[VECTORS_CHUNK_IMAGE1 + UsingBuffer0or1*VECTORS_CHUNK_IMAGE2 +
                                        CurrentcnxvectorSubChunkImg2 * VECTORS_SUBCHUNK_IMAGE2 + x];
                        //forall 364 cnxvectors "y" in chunk of image 1
                        for (int y = 0; y < VECTORS_CHUNK_IMAGE1; y++)
                        {
                            R[30] = LS[y]; //load cnxvector y to R30 ; cout <<" LS[" <<y<<"] ====== "<<endl;
                            //forall registers with cnxvector-subchunk of img 2 (~30 cnxvectors in 30 registers)
                            for(int x = 0; x < 30; x++)
                            {
                                R31 = R30 - R[x];
                                R31 = R31 * R31;
                                REDUCE(R31);
                            }
                        }
                    }
                END_BATCH();
                if (UsingBuffer0or1 == 1) return PASS; //no need to create more than 2 batches
            }
        }
    }
    cout<<"Total IO time is "<<TotalIOTime<<" ms"<<endl;
    cout<<"Total BatchExecution time is "<<TotalBatchTime<<" ms"<<endl;
    cout<<"Total TotalReductionTime time is "<<TotalReductionTime<<" ms"<<endl;
    return PASS;
}

static int connexFindMatchesPass2(int RunningMode,int LoadToRxBatchNumber,
                                    SiftDescriptors *SiftDescriptors1, SiftDescriptors *SiftDescriptors2,
                                        SiftMatches* SMs, SiftMatches* SMsFinal)
{
    int TrueMatches = 0;
    if (RunningMode == MODE_CREATE_BATCHES)
    {
        // After running, get reduced results
        //clear scores (set score to the max):
        for (int CntDescIm1 = 0; CntDescIm1 < SiftDescriptors1->RealDescriptors; CntDescIm1++)
        {
            SMs->ScoreMin[CntDescIm1] = (UINT32)-1;
            SMs->ScoreNextToMin[CntDescIm1] = (UINT32)-1;
        }

        SMs->RealMatches = 0;
    }
    else // (RunningMode == MODE_EXECUTE_FIND_MATCHES)
    {
        int TotalcnxvectorChunksImg1 = (SiftDescriptors1->RealDescriptors + VECTORS_CHUNK_IMAGE1 - 1) / VECTORS_CHUNK_IMAGE1;
        int TotalcnxvectorChunksImg2 = (SiftDescriptors2->RealDescriptors + VECTORS_CHUNK_IMAGE2 - 1) / VECTORS_CHUNK_IMAGE2;
        int TotalcnxvectorSubChunksImg2 = VECTORS_CHUNK_IMAGE2 / VECTORS_SUBCHUNK_IMAGE2;

        int RedCounter = 0;

         for(int CurrentcnxvectorChunkImg1 = 0; CurrentcnxvectorChunkImg1 < TotalcnxvectorChunksImg1; CurrentcnxvectorChunkImg1++)
         {
            // for all chunks of img 2
            for(int CurrentcnxvectorChunkImg2 = 0; CurrentcnxvectorChunkImg2 < TotalcnxvectorChunksImg2; CurrentcnxvectorChunkImg2++)
            {
                //forall subchunks of chunk of img 2
                for(int CurrentcnxvectorSubChunkImg2 = 0; CurrentcnxvectorSubChunkImg2 < TotalcnxvectorSubChunksImg2; CurrentcnxvectorSubChunkImg2++)
                    //forall 364 cnxvectors "y" in chunk of image 1
                    for (int CntDescIm1 = 0; CntDescIm1 < VECTORS_CHUNK_IMAGE1; CntDescIm1++)
                    {
                        int descIm1 = VECTORS_CHUNK_IMAGE1*CurrentcnxvectorChunkImg1 + CntDescIm1;

                        //forall registers with cnxvector-subchunk of img 2 (~30 cnxvectors in 30 registers)
                        for(int x = 0; x < 30; x++)
                        {
                            int descIm2 = CurrentcnxvectorChunkImg2*VECTORS_CHUNK_IMAGE2 + (CurrentcnxvectorSubChunkImg2 * VECTORS_SUBCHUNK_IMAGE2) + x;
                            //if (descIm1 == 0) { cout<<RedCounter<<":"<<BasicMatchRedResults[RedCounter]<<" "; if ((descIm2 & 3) == 3) cout << endl;}

                            int dsq = BasicMatchRedResults[RedCounter];
                            if (dsq < SMs->ScoreMin[descIm1])
                            {
                                SMs->ScoreNextToMin[descIm1] = SMs->ScoreMin[descIm1];
                                SMs->ScoreMin[descIm1] = dsq;

                                SMs->DescIx2ndImgNextToMin[descIm1] = SMs->DescIx2ndImgMin[descIm1];
                                SMs->DescIx2ndImgMin[descIm1] = descIm2;
                            }
                            else if (dsq < SMs->ScoreNextToMin[descIm1])
                            {
                                SMs->ScoreNextToMin[descIm1] = dsq;
                                SMs->DescIx2ndImgNextToMin[descIm1] = descIm2;
                            }

                            RedCounter++;
                        }
                    }
            }
         }

        for (int i = 0; i < SiftDescriptors1->RealDescriptors; i++)
        if (SMs->ScoreMin[i] < (FACTOR1 * SMs->ScoreNextToMin[i]) >> FACTOR2)
        {
            SMsFinal->DescIx2ndImgMin[SMsFinal->RealMatches++] = SMs->DescIx2ndImgMin[i];
        }
    }


    return PASS;
}

#define BASIC_MATCHING_BNR 0
static void FindMatchesSSE(SiftDescriptors *SDs1, SiftDescriptors *SDs2, SiftMatches* SMs);
int test_BasicMatching_All_NeonSSE()
{
    int Start;

    LoadDescriptors((char*)"data/adam1.key", &SiftDescriptors1, 0);
    LoadDescriptors((char*)"data/adam2.key", &SiftDescriptors2, 0);

    Start = GetMilliCount();
    FindMatches(&SiftDescriptors1, &SiftDescriptors2, &SM_Arm);
    cout<<"CPU-only FindMatches ran in " << GetMilliSpan(Start)<< " ms"<<flush<<endl;

    Start = GetMilliCount();
    FindMatchesSSE(&SiftDescriptors1, &SiftDescriptors2, &SM_Arm_SSE);
    cout<<"CPU-Only + SSE/NEON FindMatches ran in " << GetMilliSpan(Start)<< " ms"<<flush<<endl;

    if (PASS == CompareMatches(&SM_Arm,&SM_Arm_SSE)) cout << "Matches are a ... match ;). CPU and CPU-SSE/NEON got the same results."<<endl;
    else cout << "Match test has FAILed. CPU and CPU_SSE/NEON got different results !"<<endl;


    Start = GetMilliCount();
    connexFindMatchesPass1(MODE_CREATE_BATCHES, BASIC_MATCHING_BNR, &SiftDescriptors1, &SiftDescriptors2);
    connexFindMatchesPass2(MODE_CREATE_BATCHES, BASIC_MATCHING_BNR, &SiftDescriptors1, &SiftDescriptors2, &SM_ConnexArmMan, &SM_ConnexArm);
    cout<<"Batches were created in " << GetMilliSpan(Start)<< " ms"<<flush<<endl;

    Start = GetMilliCount();
    connexFindMatchesPass1(MODE_EXECUTE_FIND_MATCHES, BASIC_MATCHING_BNR, &SiftDescriptors1, &SiftDescriptors2);
    connexFindMatchesPass2(MODE_EXECUTE_FIND_MATCHES, BASIC_MATCHING_BNR, &SiftDescriptors1, &SiftDescriptors2, &SM_ConnexArmMan, &SM_ConnexArm);
    cout<<"connexFindMatches ran in " << GetMilliSpan(Start)<< " ms"<<flush<<endl;
    //PrintMatches(&SM_ConnexArm);

    if (PASS == CompareMatches(&SM_Arm,&SM_ConnexArm)) cout << "Matches are a ... match ;). Arm and Arm-Connex got the same results."<<endl;
    else cout << "Match test has FAILed. Arm and ConnexArm got different results !"<<endl;

	return 0;
}

//SSSE3: needed for _mm_maddubs
#ifdef __ARM_NEON__
    #include <arm_neon.h>
#else //SSE
    #include <tmmintrin.h>

    //AVX:
    //#include <immintrin.h>

    //extern void printv(__m128 m);

    #define cpuid(func,ax,bx,cx,dx)\
        __asm__ __volatile__ ("cpuid":\
        "=a" (ax), "=b" (bx), "=c" (cx), "=d" (dx) : "a" (func));

    #define UINT64 long long int
    #define INT8 char
    #define XMM_DIFF(x,y) x = _mm_sub_epi8(x, y)
    #define XMM_ABS(x) x = _mm_abs_epi8(x)
    #define XMM_MULT_REDUCTION_ADD(x) x =  _mm_maddubs_epi16(x, x)
#endif // __ARM_NEON__




#ifdef __ARM_NEON__
int MainNeonSSE()
{



}
#else
int MainNeonSSE()
{
    UINT16 dataU16[8] __declspec(align(16));
    UINT8 dataU8[16] __declspec(align(16));
    INT8 dataI8[16] __declspec(align(16));

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

    UINT8 data1[] __declspec(align(16)) = {0, 1, 0,   255, 255, 255,   6,7, 0,1,2,3,4,5,6,7};
    UINT8 data2[] __declspec(align(16)) = {1, 3, 255, 0, 255, 255,   7,6, 1,3,1,5,5,3,7,6};
    UINT8 datamaskadd[] __declspec(align(16)) = {0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F};

	__m128i m1 = _mm_load_si128((__m128i*)data1);
	__m128i m2 = _mm_load_si128((__m128i*)data2);
	//__m128i maskadd = _mm_load_si128((__m128i*)datamaskadd);

    XMM_DIFF(m1,m2);
    _mm_stream_si128((__m128i*)dataU8, m1);
    cout<<"DIFF = ";
    for (int i = 0; i < 16; i++)
    {
        cout<<(UINT16)dataU8[i]<<" ";
    }
    cout<<endl;

    m2 = _mm_sign_epi8 (m1, m2);
    //m2 = m1;
	//XMM_ABS(m2);
	//m1 = _mm_and_si128(m1, maskadd);
    _mm_stream_si128((__m128i*)dataU8, m2);
    cout<<"m2 = ";
    for (int i = 0; i < 16; i++)
    {
        cout<<(UINT16)dataU8[i]<<" ";
    }
    cout<<endl;

	XMM_MULT_REDUCTION_ADD(m1);
	m1 = _mm_maddubs_epi16(m1, m1);

    _mm_stream_si128((__m128i*)dataI8, m1);
    UINT32 sum = 0;
    for (int i = 0; i < 16; i++)
    {
        sum += dataI8[i];
        cout<<(INT16)dataI8[i]<<" ";
    }
    cout<<" SSD = "<<sum<<endl;

    sum =0;
    for (int i = 0; i < 16; i++)
        sum += (data1[i] - data2[i])*(data1[i] - data2[i]);
    cout<<" SSD = "<<sum<<endl;
	return 0;
}
#endif

#define __ARM_NEON__
#ifdef __ARM_NEON__
static void FindMatchesSSE(SiftDescriptors *SDs1, SiftDescriptors *SDs2, SiftMatches* SMs)
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

            UINT64 *src1 = (UINT64*)__builtin_assume_aligned((SDs1->SiftDescriptorsBasicFeatures[DescriptorIndex1]), 16);
            //load 128 Bytes of data (8 x (16x8) bits )
            __m128i m0 = _mm_load_si128((__m128i*)(src1));
            __m128i m1 = _mm_load_si128((__m128i*)(src1 + 2));
            __m128i m2 = _mm_load_si128((__m128i*)(src1 + 4));
            __m128i m3 = _mm_load_si128((__m128i*)(src1 + 6));
            __m128i m4 = _mm_load_si128((__m128i*)(src1 + 8));
            __m128i m5 = _mm_load_si128((__m128i*)(src1 + 10));
            __m128i m6 = _mm_load_si128((__m128i*)(src1 + 12));
            __m128i m7 = _mm_load_si128((__m128i*)(src1 + 14));


        for (DescriptorIndex2 =0; DescriptorIndex2 < SDs2->RealDescriptors; DescriptorIndex2++)
	    {
	        dsq = 0;
            UINT64 *src2 = (UINT64*)__builtin_assume_aligned((SDs2->SiftDescriptorsBasicFeatures[DescriptorIndex2]), 16);
/*
            //load 128 Bytes of data (8 x (16x8) bits )
            __m128i m8 = _mm_load_si128((__m128i*)(src1));
            __m128i m9 = _mm_load_si128((__m128i*)(src1 + 2));
            __m128i m10 = _mm_load_si128((__m128i*)(src1 + 4));
            __m128i m11 = _mm_load_si128((__m128i*)(src1 + 6));
            __m128i m12 = _mm_load_si128((__m128i*)(src1 + 8));
            __m128i m13 = _mm_load_si128((__m128i*)(src1 + 10));
            __m128i m14 = _mm_load_si128((__m128i*)(src1 + 12));
            __m128i m15 = _mm_load_si128((__m128i*)(src1 + 14));


            XMM_DIFF(m8,m0);
            XMM_DIFF(m9,m1);
            XMM_DIFF(m10,m2);
            XMM_DIFF(m11,m3);

            XMM_DIFF(m12,m4);
            XMM_DIFF(m13,m5);
            XMM_DIFF(m14,m6);
            XMM_DIFF(m15,m7);


            XMM_ABS(m8);
            XMM_ABS(m9);
            XMM_ABS(m10);
            XMM_ABS(m11);
            XMM_ABS(m12);
            XMM_ABS(m13);
            XMM_ABS(m14);
            XMM_ABS(m15);

            XMM_MULT_REDUCTION_ADD(m8);
            XMM_MULT_REDUCTION_ADD(m9);
            XMM_MULT_REDUCTION_ADD(m10);
            XMM_MULT_REDUCTION_ADD(m11);
            XMM_MULT_REDUCTION_ADD(m12);
            XMM_MULT_REDUCTION_ADD(m13);
            XMM_MULT_REDUCTION_ADD(m14);
            XMM_MULT_REDUCTION_ADD(m15);

*/
            UINT16 mults[8*8] __declspec(align(16));
/*
            _mm_store_si128((__m128i*)&mults[0], m8);
            _mm_store_si128((__m128i*)&mults[8], m9);
            _mm_store_si128((__m128i*)&mults[16], m10);
            _mm_store_si128((__m128i*)&mults[24], m11);

            _mm_store_si128((__m128i*)&mults[32], m12);
            _mm_store_si128((__m128i*)&mults[40], m13);
            _mm_store_si128((__m128i*)&mults[48], m14);
            _mm_store_si128((__m128i*)&mults[56], m15);
*/
            for (int i = 0; i < 64; i++)
                dsq += mults[i];
            /*
            for (FeatIndex = 0; FeatIndex < FEATURES_PER_DESCRIPTOR; FeatIndex+= 8)
            {
                INT32 sq = (SDs1->SiftDescriptorsBasicFeatures[DescriptorIndex1][FeatIndex] -
                            SDs2->SiftDescriptorsBasicFeatures[DescriptorIndex2][FeatIndex]);
                dsq += sq*sq;
            }
            */

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

#else
static void FindMatchesSSE(SiftDescriptors *SDs1, SiftDescriptors *SDs2, SiftMatches* SMs)
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

            UINT64 *src1 = (UINT64*)__builtin_assume_aligned((SDs1->SiftDescriptorsBasicFeatures[DescriptorIndex1]), 16);
            //load 128 Bytes of data (8 x (16x8) bits )
            __m128i m0 = _mm_load_si128((__m128i*)(src1));
            __m128i m1 = _mm_load_si128((__m128i*)(src1 + 2));
            __m128i m2 = _mm_load_si128((__m128i*)(src1 + 4));
            __m128i m3 = _mm_load_si128((__m128i*)(src1 + 6));
            __m128i m4 = _mm_load_si128((__m128i*)(src1 + 8));
            __m128i m5 = _mm_load_si128((__m128i*)(src1 + 10));
            __m128i m6 = _mm_load_si128((__m128i*)(src1 + 12));
            __m128i m7 = _mm_load_si128((__m128i*)(src1 + 14));


        for (DescriptorIndex2 =0; DescriptorIndex2 < SDs2->RealDescriptors; DescriptorIndex2++)
	    {
	        dsq = 0;
            UINT64 *src2 = (UINT64*)__builtin_assume_aligned((SDs2->SiftDescriptorsBasicFeatures[DescriptorIndex2]), 16);
/*
            //load 128 Bytes of data (8 x (16x8) bits )
            __m128i m8 = _mm_load_si128((__m128i*)(src1));
            __m128i m9 = _mm_load_si128((__m128i*)(src1 + 2));
            __m128i m10 = _mm_load_si128((__m128i*)(src1 + 4));
            __m128i m11 = _mm_load_si128((__m128i*)(src1 + 6));
            __m128i m12 = _mm_load_si128((__m128i*)(src1 + 8));
            __m128i m13 = _mm_load_si128((__m128i*)(src1 + 10));
            __m128i m14 = _mm_load_si128((__m128i*)(src1 + 12));
            __m128i m15 = _mm_load_si128((__m128i*)(src1 + 14));


            XMM_DIFF(m8,m0);
            XMM_DIFF(m9,m1);
            XMM_DIFF(m10,m2);
            XMM_DIFF(m11,m3);

            XMM_DIFF(m12,m4);
            XMM_DIFF(m13,m5);
            XMM_DIFF(m14,m6);
            XMM_DIFF(m15,m7);


            XMM_ABS(m8);
            XMM_ABS(m9);
            XMM_ABS(m10);
            XMM_ABS(m11);
            XMM_ABS(m12);
            XMM_ABS(m13);
            XMM_ABS(m14);
            XMM_ABS(m15);

            XMM_MULT_REDUCTION_ADD(m8);
            XMM_MULT_REDUCTION_ADD(m9);
            XMM_MULT_REDUCTION_ADD(m10);
            XMM_MULT_REDUCTION_ADD(m11);
            XMM_MULT_REDUCTION_ADD(m12);
            XMM_MULT_REDUCTION_ADD(m13);
            XMM_MULT_REDUCTION_ADD(m14);
            XMM_MULT_REDUCTION_ADD(m15);

*/
            UINT16 mults[8*8] __declspec(align(16));
/*
            _mm_store_si128((__m128i*)&mults[0], m8);
            _mm_store_si128((__m128i*)&mults[8], m9);
            _mm_store_si128((__m128i*)&mults[16], m10);
            _mm_store_si128((__m128i*)&mults[24], m11);

            _mm_store_si128((__m128i*)&mults[32], m12);
            _mm_store_si128((__m128i*)&mults[40], m13);
            _mm_store_si128((__m128i*)&mults[48], m14);
            _mm_store_si128((__m128i*)&mults[56], m15);
*/
            for (int i = 0; i < 64; i++)
                dsq += mults[i];
            /*
            for (FeatIndex = 0; FeatIndex < FEATURES_PER_DESCRIPTOR; FeatIndex+= 8)
            {
                INT32 sq = (SDs1->SiftDescriptorsBasicFeatures[DescriptorIndex1][FeatIndex] -
                            SDs2->SiftDescriptorsBasicFeatures[DescriptorIndex2][FeatIndex]);
                dsq += sq*sq;
            }
            */

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
#endif // __SSE__
