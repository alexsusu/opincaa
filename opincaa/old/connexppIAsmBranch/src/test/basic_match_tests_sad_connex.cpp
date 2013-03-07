
/*
 *
 * File: basic_match_tests.cpp
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

#include <omp.h>

//#include <unistd.h>     /* Symbolic Constants */
//#include <sys/types.h>  /* Primitive System Data Types */
//#include <errno.h>      /* Errors */
//#include <stdio.h>      /* Input/Output */
//#include <sys/wait.h>   /* Wait for Process Termination */
#include <stdlib.h>

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
    SiftDescriptor SD[MAX_DESCRIPTORS];
    UINT16 SiftDescriptorsBasicFeatures[MAX_DESCRIPTORS][FEATURES_PER_DESCRIPTOR];
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

static SiftDescriptors SiftDescriptors1;
static SiftDescriptors SiftDescriptors2;
static SiftMatches SM_Arm;
static SiftMatches SM_Arm_OMP;
static SiftMatches SM_ConnexArm;
static SiftMatches SM_ConnexArm2;
static SiftMatches SM_ConnexArmMan;
static SiftMatches SM_ConnexArmMan2;
static UINT_RED_REG_VAL *BasicMatchRedResults;

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
            //printf("SDs[%d]->SD.BasicFeatures[%d] = %d \n",
              //     DescriptorIndex,
                //   FeatIndex,
                   //SDs->SiftDescriptorsBasicFeatures[DescriptorIndex][FeatIndex]);
            printf(" %d \n",
                   SDs->SiftDescriptorsBasicFeatures[DescriptorIndex][FeatIndex]);
    }
}

static void PrintMatches(SiftMatches *SMs)
{
    int MatchesIndex=0;
    printf("\n");
    for(MatchesIndex =0; MatchesIndex < SMs->RealMatches; MatchesIndex++)
        printf("Matches[%d].DescriptorIndexInSecondImage = %d \n", MatchesIndex, SMs->DescIx2ndImgMin[MatchesIndex]);
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
                result = fscanf(fp,"%hu",&SDs->SiftDescriptorsBasicFeatures[DescriptorIndex][FeatIndex]);

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
		int minIndex=0;
		int nexttominIndex;
		UINT32 distsq1, distsq2;
		distsq1 = (UINT32)-1;
		distsq2 = (UINT32)-1;

        for (DescriptorIndex2 =0; DescriptorIndex2 < SDs2->RealDescriptors; DescriptorIndex2++)
	    {
            UINT32 dsq = 0;

			int FeatIndex;
            for (FeatIndex = 0; FeatIndex < FEATURES_PER_DESCRIPTOR; FeatIndex++)
            {
                INT32 sq = abs(SDs1->SiftDescriptorsBasicFeatures[DescriptorIndex1][FeatIndex] -
                            SDs2->SiftDescriptorsBasicFeatures[DescriptorIndex2][FeatIndex]);
                dsq += sq;
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
    }
}

static inline unsigned int SAD16_Distance(unsigned short int *set1, unsigned short int *set2)
{
    unsigned int ssd = 0;
    for (int i=0; i < 128; i++)
        ssd += abs((*set1++) - (*set2++));
    return ssd;
}

static void FindMatchesOMP(SiftDescriptors *SDs1, SiftDescriptors *SDs2, SiftMatches* SMs, int threads)
{
    //area for parallel real matches finding
    static SiftMatches SM_Arm_OMP_man;
    for (int i =0 ;i < SDs1->RealDescriptors; i++) SM_Arm_OMP_man.DescIx2ndImgMin[i] = (UINT32)-1;

    SMs->RealMatches = 0;
    int *dsq = (int*) malloc(SDs1->RealDescriptors*SDs2->RealDescriptors*sizeof(int));
//    for (int er=0; er < SDs1->RealDescriptors; er++) dsq[er] = new int[SDs2->RealDescriptors];

    if (threads !=0) omp_set_num_threads(threads);
        #pragma omp parallel for
        for (int DescriptorIndex1 =0; DescriptorIndex1 < SDs1->RealDescriptors; DescriptorIndex1++)
        {
                for (int DescriptorIndex2 =0; DescriptorIndex2 < SDs2->RealDescriptors; DescriptorIndex2++)
                {
                    dsq[DescriptorIndex1*SDs2->RealDescriptors+DescriptorIndex2] = SAD16_Distance(SDs1->SiftDescriptorsBasicFeatures[DescriptorIndex1],
                                                                             SDs2->SiftDescriptorsBasicFeatures[DescriptorIndex2]);
                }
        }

        int TimeStart = GetMilliCount();
        #pragma omp parallel for
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
                SM_Arm_OMP_man.DescIx2ndImgMin[DescriptorIndex1] = minIndex;
                //SMs->DescIx2ndImgMin[SMs->RealMatches++] = minIndex;
        }

        //serial region
        for (int i =0 ;i < SDs1->RealDescriptors; i++)
            if (SM_Arm_OMP_man.DescIx2ndImgMin[i] != (UINT32)-1)
            {
                SMs->DescIx2ndImgMin[SMs->RealMatches++] = SM_Arm_OMP_man.DescIx2ndImgMin[i];
            }

        cout<<"  |    Total FindGoodMatch time is "<<GetMilliSpan(TimeStart)<<" ms"<<endl;

//        for (int er=0; er < SDs1->RealDescriptors; er++) delete(dsq[er]);
        free(dsq);
}

/*
    Out of 1024 LocalStore locations, each having 128 Bytes:
    (fortunately a SIFT descriptor also has 128 features of 1 Bytes each)
*/
#define VECTORS_CHUNK_IMAGE1 364  //  364 cnxvectors - reserved for img1 (cnxvectorS_CHUNK_IMG1)
#define VECTORS_CHUNK_IMAGE2 (11*VECTORS_SUBCHUNK_IMAGE2) // 330 cnxvectors - reserved for img2 - work (cnxvectorS_CHUNK_IMG2)
//another 330 cnxvectors - reserved for img2 - transfer with IO

#define VECTORS_SUBCHUNK_IMAGE2 28 // 30 cnxvectors from VECTORS_CHUNK_IMAGE2 to be cached for inner loop of distance calculation

static io_unit IOU_CVCI1;
static io_unit IOU_CVCI2;
#define MODE_CREATE_BATCHES 0
#define MODE_EXECUTE_FIND_MATCHES 1

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
                EXECUTE_IN_ALL(
                    R29 = 0;
                    //forall subchunks of chunk of img 2
                    for(int CurrentcnxvectorSubChunkImg2 = 0; CurrentcnxvectorSubChunkImg2 < TotalcnxvectorSubChunksImg2; CurrentcnxvectorSubChunkImg2++)
                    {
                        //forall cnxvectors in subchunk of chunk of img (~30 cnxvectors) load cnxvector x to Rx
                        for(int x = 0; x < VECTORS_SUBCHUNK_IMAGE2; x++)
                            R[x] = LS[VECTORS_CHUNK_IMAGE1 + UsingBuffer0or1*VECTORS_CHUNK_IMAGE2 +
                                        CurrentcnxvectorSubChunkImg2 * VECTORS_SUBCHUNK_IMAGE2 + x];
                        //forall 364 cnxvectors "y" in chunk of image 1
                        for (int y = 0; y < VECTORS_CHUNK_IMAGE1; y++)
                        {
                            R[VECTORS_SUBCHUNK_IMAGE2] = LS[y]; //load cnxvector y to R30 ; cout <<" LS[" <<y<<"] ====== "<<endl;
                            //forall registers with cnxvector-subchunk of img 2 (~30 cnxvectors in 30 registers)
                            for(int x = 0; x < VECTORS_SUBCHUNK_IMAGE2; x++)
                            {
                                R30 = R[VECTORS_SUBCHUNK_IMAGE2] - R[x];
                                R31 = R30 < R29;
                                NOP;
                                EXECUTE_WHERE_LT
                                (
                                  R30 = R[x] - R[VECTORS_SUBCHUNK_IMAGE2];
                                )
                                EXECUTE_IN_ALL(REDUCE(R30));
                            }
                        }
                    })
                END_BATCH();
                if (UsingBuffer0or1 == 1) return PASS; //no need to create more than 2 batches
            }
        }
    }
    cout<<"   ___"<<endl;
    cout<<"  |___ Total IO time is "<<TotalIOTime<<" ms"<<endl;
    cout<<"  |___ Total BatchExecution time is "<<TotalBatchTime<<" ms"<<endl;
    cout<<"  |    Total TotalReductionTime time is "<<TotalReductionTime<<" ms"<<endl;
    return PASS;
}

static int connexFindMatchesPass2(int RunningMode,int LoadToRxBatchNumber,
                                    SiftDescriptors *SiftDescriptors1, SiftDescriptors *SiftDescriptors2,
                                        SiftMatches* SMs, SiftMatches* SMsFinal)
{
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
        int TimeStart = GetMilliCount();
        //cout<<SiftDescriptors1->RealDescriptors<<" x "<<SiftDescriptors2->RealDescriptors<<endl;
        int TotalcnxvectorChunksImg1 = (SiftDescriptors1->RealDescriptors + VECTORS_CHUNK_IMAGE1 - 1) / VECTORS_CHUNK_IMAGE1;
        int TotalcnxvectorChunksImg2 = (SiftDescriptors2->RealDescriptors + VECTORS_CHUNK_IMAGE2 - 1) / VECTORS_CHUNK_IMAGE2;
        int TotalcnxvectorSubChunksImg2 = VECTORS_CHUNK_IMAGE2 / VECTORS_SUBCHUNK_IMAGE2;

        int RedCounter = 0;
        int CounttimesLess = 0;

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
                        for(int x = 0; x < VECTORS_SUBCHUNK_IMAGE2; x++)
                        {
                            int descIm2 = CurrentcnxvectorChunkImg2*VECTORS_CHUNK_IMAGE2 + (CurrentcnxvectorSubChunkImg2 * VECTORS_SUBCHUNK_IMAGE2) + x;
                            if ((descIm1 < SiftDescriptors1->RealDescriptors) && (descIm2 < SiftDescriptors2->RealDescriptors))
                            {
                                //if (descIm1 == 0) { cout<<RedCounter<<":"<<BasicMatchRedResults[RedCounter]<<" "; if ((descIm2 & 3) == 3) cout << endl;}

                                UINT_RED_REG_VAL dsq = BasicMatchRedResults[RedCounter];
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
                                CounttimesLess++;
                            }
                            RedCounter++;
                        }
                    }
            }
         }

        for (int i = 0; i < SiftDescriptors1->RealDescriptors; i++)
        {
            if (SMs->ScoreMin[i] < (FACTOR1 * SMs->ScoreNextToMin[i]) >> FACTOR2)
                SMsFinal->DescIx2ndImgMin[SMsFinal->RealMatches++] = SMs->DescIx2ndImgMin[i];
        }
        //cout<<"CounttimesLess "<<CounttimesLess<<endl;
        cout<<"  |    Total FindGoodMatch time is "<<GetMilliSpan(TimeStart)<<" ms"<<endl;
    }
    return PASS;
}

#define JMP_VECTORS_CHUNK_IMAGE1 364
#define JMP_VECTORS_SUBCHUNK_IMAGE2 24 //keep it even, for easy double buffering
#define JMP_VECTORS_CHUNK_IMAGE2 (JMP_VECTORS_SUBCHUNK_IMAGE2*13)

#undef VECTORS_CHUNK_IMAGE1
#undef VECTORS_CHUNK_IMAGE2
#undef VECTORS_SUBCHUNK_IMAGE2

/**

Used resorces:
--------------
>> LS <<
Localstore 0...363 (JMP_VECTORS_CHUNK_IMAGE1), has a chunk of img1
Localstore 364...~700 (JMP_VECTORS_CHUNK_IMAGE1+JMP_VECTORS_CHUNK_IMAGE2) has first chunk of img2
Localstore     ...1023 has the second chunk of img2

>> REG <<
R0 ... R25 filled with one img2 subchunk
R26, reserved for double buffering
R27 , reserved as dest for LT instruction (R27 = R31 < R28)
R28 = 0; // reserved for 0: helps in comparison with 0, for absolute value {if (a-b < 0) then return (b-a) else return (a-b);}
R29 = 1;//reserved for increment with one
R30 is reserved for localstore loading location (looped jmp variable)
R31 = we reduce this for the SAD
*/

static int connexJmpFindMatchesPass1(int RunningMode,int LoadToRxBatchNumber,
                                    SiftDescriptors *SiftDescriptors1, SiftDescriptors *SiftDescriptors2)
{
    int CurrentcnxvectorChunkImg1, CurrentcnxvectorChunkImg2;
    int TotalcnxvectorChunksImg1 = (SiftDescriptors1->RealDescriptors + JMP_VECTORS_CHUNK_IMAGE1 - 1) / JMP_VECTORS_CHUNK_IMAGE1;
    int TotalcnxvectorChunksImg2 = (SiftDescriptors2->RealDescriptors + JMP_VECTORS_CHUNK_IMAGE2 - 1) / JMP_VECTORS_CHUNK_IMAGE2;
    int TotalcnxvectorSubChunksImg2 = JMP_VECTORS_CHUNK_IMAGE2 / JMP_VECTORS_SUBCHUNK_IMAGE2;
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
            IOU_CVCI1.preWritecnxvectors(0,SiftDescriptors1->SiftDescriptorsBasicFeatures[JMP_VECTORS_CHUNK_IMAGE1*CurrentcnxvectorChunkImg1],JMP_VECTORS_CHUNK_IMAGE1);
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
                    IOU_CVCI2.preWritecnxvectors(JMP_VECTORS_CHUNK_IMAGE1 + UsingBuffer0or1*JMP_VECTORS_CHUNK_IMAGE2,
                                                    SiftDescriptors2->SiftDescriptorsBasicFeatures[JMP_VECTORS_CHUNK_IMAGE2*CurrentcnxvectorChunkImg2],
                                                        JMP_VECTORS_CHUNK_IMAGE2);
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
                    int ExpectedBytesOfReductions = BYTES_IN_DWORD* JMP_VECTORS_CHUNK_IMAGE1 * JMP_VECTORS_CHUNK_IMAGE2;
                    TimeStart = GetMilliCount();
                    int RealBytesOfReductions = GET_MULTIRED_RESULT(BasicMatchRedResults +
                                        JMP_VECTORS_CHUNK_IMAGE1 * (TotalcnxvectorChunksImg2*JMP_VECTORS_CHUNK_IMAGE2) * CurrentcnxvectorChunkImg1 +
                                        JMP_VECTORS_CHUNK_IMAGE1 * JMP_VECTORS_CHUNK_IMAGE2 * CurrentcnxvectorChunkImg2,
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
                    R28 = 0;
                    R29 = 1;//reserved for increment with one

                    //forall subchunks of chunk of img 2
                    for(int CurrentcnxvectorSubChunkImg2 = 0; CurrentcnxvectorSubChunkImg2 < TotalcnxvectorSubChunksImg2; CurrentcnxvectorSubChunkImg2++)
                    {
                        //forall cnxvectors in subchunk of chunk of img (~30 cnxvectors) load cnxvector x to Rx

                        for(int x = 0; x < JMP_VECTORS_SUBCHUNK_IMAGE2; x++)
                            R[x] = LS[JMP_VECTORS_CHUNK_IMAGE1 + UsingBuffer0or1*JMP_VECTORS_CHUNK_IMAGE2 +
                                        CurrentcnxvectorSubChunkImg2 * JMP_VECTORS_SUBCHUNK_IMAGE2 + x];

                        //forall 364 cnxvectors "y" in chunk of image 1
                        //for (int y = 0; y < JMP_VECTORS_CHUNK_IMAGE1; y++)

                       R30 = 0;/* R30 is reserved for localstore loading location */
                       REPEAT_X_TIMES(JMP_VECTORS_CHUNK_IMAGE1,

                            //R[JMP_VECTORS_SUBCHUNK_IMAGE2] = LS[y]; //load cnxvector y to R30 ; cout <<" LS[" <<y<<"] ====== "<<endl;
                            R[JMP_VECTORS_SUBCHUNK_IMAGE2] = LS[R30]; //load cnxvector y to R30 ; cout <<" LS[" <<y<<"] ====== "<<endl;
                            R30 = R30 + R29;
                            //forall registers with cnxvector-subchunk of img 2 (~30 cnxvectors in 30 registers)
                            for(int x = 0; x < JMP_VECTORS_SUBCHUNK_IMAGE2; x+=2)
                            {
                                /*
                                R0 ... R26 filled with one img2 subchunk
                                R27 , reserved as dest for LT instruction (R27 = R31 < R28)
                                R28 = 0; // reserved for 0: helps in comparison with 0, for absolute value {if (a-b < 0) then return (b-a) else return (a-b);}
                                R29 = 1;//reserved for increment with one
                                R30 is reserved for localstore loading location (looped jmp variable)
                                R31 = we reduce this for the SAD
                                */

                                R31 = R[JMP_VECTORS_SUBCHUNK_IMAGE2] - R[x];
                                R26 = R[JMP_VECTORS_SUBCHUNK_IMAGE2] - R[x+1];
                                R27 = R31 < R28;
                                R27 = R26 < R28;
                                EXECUTE_WHERE_LT(R31 = R[x] - R[JMP_VECTORS_SUBCHUNK_IMAGE2];) // this is the lt from R31 < R28;
                                EXECUTE_WHERE_LT(R26 = R[x+1] - R[JMP_VECTORS_SUBCHUNK_IMAGE2];) // this is the lt from RR26 < 28;
                                EXECUTE_IN_ALL(REDUCE(R31);REDUCE(R26);)
                            }
                        )
                    }
                END_BATCH();
                if (UsingBuffer0or1 == 1) return PASS; //no need to create more than 2 batches
            }
        }
    }
    //DEASM_BATCH(0);
    cout<<"   ___"<<endl;
    cout<<"  |___ Total IO time is "<<TotalIOTime<<" ms"<<endl;
    cout<<"  |___ Total BatchExecution time is "<<TotalBatchTime<<" ms"<<endl;
    cout<<"  |    Total TotalReductionTime time is "<<TotalReductionTime<<" ms"<<endl;

    return PASS;
}

static void IntProofConcept()
{
    BEGIN_BATCH(5);
    R30 = 0;
    //for (int i=0; i <2; i++)
    {
        SET_JMP_LABEL(0);
        //R[JMP_VECTORS_SUBCHUNK_IMAGE2] = LS[y]; //load cnxvector y to R30 ; cout <<" LS[" <<y<<"] ====== "<<endl;
        R[JMP_VECTORS_SUBCHUNK_IMAGE2] = LS[R30]; //load cnxvector y to R30 ; cout <<" LS[" <<y<<"] ====== "<<endl;
        R30 = R30 + R29;
        //forall registers with cnxvector-subchunk of img 2 (~30 cnxvectors in 30 registers)
        for(int x = 0; x < 1; x++)
        {
            R31 = R[JMP_VECTORS_SUBCHUNK_IMAGE2] - R[x];
            R31 = R31 * R31;
            REDUCE(R31);
        }
        JMP_TIMES_TO_LABEL(1,0);
    }
    {
        SET_JMP_LABEL(0);
        //R[JMP_VECTORS_SUBCHUNK_IMAGE2] = LS[y]; //load cnxvector y to R30 ; cout <<" LS[" <<y<<"] ====== "<<endl;
        R[JMP_VECTORS_SUBCHUNK_IMAGE2] = LS[R30]; //load cnxvector y to R30 ; cout <<" LS[" <<y<<"] ====== "<<endl;
        R30 = R30 + R29;
        //forall registers with cnxvector-subchunk of img 2 (~30 cnxvectors in 30 registers)
        for(int x = 0; x < 1; x++)
        {
            R31 = R[JMP_VECTORS_SUBCHUNK_IMAGE2] - R[x];
            R31 = R31 * R31;
            REDUCE(R31);
        }
        JMP_TIMES_TO_LABEL(1,0);
    }

    END_BATCH(5);
    DEASM_BATCH(5);

    EXECUTE_BATCH(5);
    int ExpectedBytesOfReductions = 1*2*4*10;
    int RealBytesOfReductions = GET_MULTIRED_RESULT(BasicMatchRedResults, ExpectedBytesOfReductions);
    cout<<"RealBytesOfReductions = "<<RealBytesOfReductions<<endl;
    cout<<"ExpectedBytesOfReductions = "<<ExpectedBytesOfReductions<<endl;


}

static int connexJmpFindMatchesPass2(int RunningMode,int LoadToRxBatchNumber,
                                    SiftDescriptors *SiftDescriptors1, SiftDescriptors *SiftDescriptors2,
                                        SiftMatches* SMs, SiftMatches* SMsFinal)
{
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
        int TimeStart = GetMilliCount();
        int TotalcnxvectorChunksImg1 = (SiftDescriptors1->RealDescriptors + JMP_VECTORS_CHUNK_IMAGE1 - 1) / JMP_VECTORS_CHUNK_IMAGE1;
        int TotalcnxvectorChunksImg2 = (SiftDescriptors2->RealDescriptors + JMP_VECTORS_CHUNK_IMAGE2 - 1) / JMP_VECTORS_CHUNK_IMAGE2;
        int TotalcnxvectorSubChunksImg2 = JMP_VECTORS_CHUNK_IMAGE2 / JMP_VECTORS_SUBCHUNK_IMAGE2;

        int RedCounter = 0;

         for(int CurrentcnxvectorChunkImg1 = 0; CurrentcnxvectorChunkImg1 < TotalcnxvectorChunksImg1; CurrentcnxvectorChunkImg1++)
         {
            // for all chunks of img 2
            for(int CurrentcnxvectorChunkImg2 = 0; CurrentcnxvectorChunkImg2 < TotalcnxvectorChunksImg2; CurrentcnxvectorChunkImg2++)
            {
                //forall subchunks of chunk of img 2
                for(int CurrentcnxvectorSubChunkImg2 = 0; CurrentcnxvectorSubChunkImg2 < TotalcnxvectorSubChunksImg2; CurrentcnxvectorSubChunkImg2++)
                    //forall 364 cnxvectors "y" in chunk of image 1
                    for (int CntDescIm1 = 0; CntDescIm1 < JMP_VECTORS_CHUNK_IMAGE1; CntDescIm1++)
                    {
                        int descIm1 = JMP_VECTORS_CHUNK_IMAGE1*CurrentcnxvectorChunkImg1 + CntDescIm1;

                        //forall registers with cnxvector-subchunk of img 2 (~30 cnxvectors in 30 registers)
                        for(int x = 0; x < JMP_VECTORS_SUBCHUNK_IMAGE2; x++)
                        {
                            int descIm2 = CurrentcnxvectorChunkImg2*JMP_VECTORS_CHUNK_IMAGE2 +
                                            (CurrentcnxvectorSubChunkImg2 * JMP_VECTORS_SUBCHUNK_IMAGE2) + x;
                            //if (descIm1 == 0) { cout<<RedCounter<<":"<<BasicMatchRedResults[RedCounter]<<" "; if ((descIm2 & 3) == 3) cout << endl;}

                            if ((descIm1 < SiftDescriptors1->RealDescriptors) && (descIm2 < SiftDescriptors2->RealDescriptors))
                            {
                                UINT_RED_REG_VAL dsq = BasicMatchRedResults[RedCounter];
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
                            }
                            RedCounter++;
                        }
                    }
            }
         }

        SiftMatches SMsFinalman;
        for (int i=0; i < SiftDescriptors1->RealDescriptors; i++) SMsFinalman.DescIx2ndImgMin[i] = (UINT32)-1;

        for (int i = 0; i < SiftDescriptors1->RealDescriptors; i++)
        if (SMs->ScoreMin[i] < (FACTOR1 * SMs->ScoreNextToMin[i]) >> FACTOR2)
        {
           SMsFinalman.DescIx2ndImgMin[i] = SMs->DescIx2ndImgMin[i];
        }

        for (int i=0; i < SiftDescriptors1->RealDescriptors; i++)
            if (SMsFinalman.DescIx2ndImgMin[i] != (UINT32)-1)
                SMsFinal->DescIx2ndImgMin[SMsFinal->RealMatches++] = SMsFinalman.DescIx2ndImgMin[i];

        cout<<"  |    Total FindGoodMatch time is "<<GetMilliSpan(TimeStart)<<" ms"<<endl;
    }


    return PASS;
}

#define BASIC_MATCHING_BNR 0
#define JMP_BASIC_MATCHING_BNR 2
int test_BasicMatching_All_SAD(char* fn1, char* fn2, FILE* logfile)
{
    int Start, Delta;

    LoadDescriptors(fn1, &SiftDescriptors1, 0);
    LoadDescriptors(fn2, &SiftDescriptors2, 0);

    BasicMatchRedResults = (UINT_RED_REG_VAL*)malloc(MAX_REDUCES * sizeof(UINT_RED_REG_VAL));
    if (BasicMatchRedResults == NULL) {cout<<"Could not allocate memory for reductions "<<endl;return 0;};

    float BruteMatches = SiftDescriptors1.RealDescriptors * SiftDescriptors2.RealDescriptors;

    /*
    Start = GetMilliCount();
    if (PASS != kernel_acc::storeKernel("database/BasicMatchingA.ker", BASIC_MATCHING_BNR))
        cout<<"Could not store kernel "<<endl;

    if (PASS != kernel_acc::storeKernel("database/BasicMatchingB.ker", BASIC_MATCHING_BNR+1))
        cout<<"Could not store kernel "<<endl;

    cout<<"Kernels were stored in " << GetMilliSpan(Start)<< " ms"<<endl;

    Start = GetMilliCount();
    if (PASS != kernel_acc::loadKernel((char*)"database/BasicMatchingA.ker", BASIC_MATCHING_BNR+2))
        cout<<"Could not load kernel "<<endl;
    if (PASS != kernel_acc::loadKernel("database/BasicMatchingB.ker", BASIC_MATCHING_BNR+3))
        cout<<"Could not load kernel "<<endl;

    cout<<"Batches were loaded in " << GetMilliSpan(Start)<< " ms"<<endl;
    //cout<<"Could not load batches "<<endl;
    */

    cout<<endl<<"Starting SAD16: "<<endl;
    fprintf(logfile, "\nStarting SAD16:\n");

    #ifdef __ARM_NEON__ //run only on zedboard
    // STEP1: Compute on ConnexS no jump
    Start = GetMilliCount();
    connexFindMatchesPass1(MODE_CREATE_BATCHES, BASIC_MATCHING_BNR, &SiftDescriptors1, &SiftDescriptors2);
    connexFindMatchesPass2(MODE_CREATE_BATCHES, BASIC_MATCHING_BNR, &SiftDescriptors1, &SiftDescriptors2, &SM_ConnexArmMan, &SM_ConnexArm);
    cout<<"  ConnexS-unrolled batches were created in " << GetMilliSpan(Start)<< " ms"<<flush<<endl;

    Start = GetMilliCount();
    connexFindMatchesPass1(MODE_EXECUTE_FIND_MATCHES, BASIC_MATCHING_BNR, &SiftDescriptors1, &SiftDescriptors2);
    connexFindMatchesPass2(MODE_EXECUTE_FIND_MATCHES, BASIC_MATCHING_BNR, &SiftDescriptors1, &SiftDescriptors2, &SM_ConnexArmMan, &SM_ConnexArm);
    Delta  = GetMilliSpan(Start);
    cout<<"> ConnexS-unrolled connexFindMatches ran in " << Delta << " ms ("<< BruteMatches/Delta/1000 <<" MM/s)"<<flush<<endl;
    fprintf(logfile, "ConnexS-unrolled_ran_in_time %d %f MM/s \n", Delta, BruteMatches/Delta/1000);


    // STEP2 Compute on Connex-S with JMP
    Start = GetMilliCount();
    connexJmpFindMatchesPass1(MODE_CREATE_BATCHES, JMP_BASIC_MATCHING_BNR, &SiftDescriptors1, &SiftDescriptors2);
    connexJmpFindMatchesPass2(MODE_CREATE_BATCHES, JMP_BASIC_MATCHING_BNR, &SiftDescriptors1, &SiftDescriptors2, &SM_ConnexArmMan2, &SM_ConnexArm2);
    cout<<"  ConnexS-JMP Batches were created in "<< GetMilliSpan(Start)<< " ms"<<flush<<endl;

    /*
        if (PASS != kernel_acc::storeKernel("database/connexJmpFindMatchesPass1_b1.ker", JMP_BASIC_MATCHING_BNR))
            cout<<"Could not store kernel "<<endl;
        if (PASS != kernel_acc::storeKernel("database/connexJmpFindMatchesPass1_b2.ker", JMP_BASIC_MATCHING_BNR+1))
            cout<<"Could not store kernel "<<endl;
    */

    Start = GetMilliCount();
    connexJmpFindMatchesPass1(MODE_EXECUTE_FIND_MATCHES, JMP_BASIC_MATCHING_BNR, &SiftDescriptors1, &SiftDescriptors2);
    connexJmpFindMatchesPass2(MODE_EXECUTE_FIND_MATCHES, JMP_BASIC_MATCHING_BNR, &SiftDescriptors1, &SiftDescriptors2, &SM_ConnexArmMan2, &SM_ConnexArm2);
    Delta  = GetMilliSpan(Start);
    cout<<"> ConnexS-JMP connexFindMatches ran in " << Delta << " ms ("<< BruteMatches/Delta/1000 <<" MM/s)"<<flush<<endl;
    fprintf(logfile, "ConnexS-JMP_ran_in_time %d %f MM/s \n", Delta, BruteMatches/Delta/1000);

    /* STEP3: Compare Connex-S (noJMP) with JMP */
    if (PASS == CompareMatches(&SM_ConnexArm2,&SM_ConnexArm)) cout << "OK ! Arm == Arm-Connex"<<endl<<endl;
    else cout << "Match test has FAILed. Arm and ConnexArm got different results !"<<endl<<endl;

    #endif
    free(BasicMatchRedResults);

    /* STEP4: Compute on cpu-only  */
    Start = GetMilliCount();
    FindMatches(&SiftDescriptors1, &SiftDescriptors2, &SM_Arm);
    Delta  = GetMilliSpan(Start);
    cout<<"> cpu-only FindMatches ran in " << Delta << " ms ("<< BruteMatches/Delta/1000 <<" MM/s)"<<flush<<endl;
    fprintf(logfile, "cpu-only_ran_in_time %d %f MM/s \n", Delta, BruteMatches/Delta/1000);

    #ifdef __ARM_NEON__ //run only on zedboard
    /* STEP5: Compare Connex-S (noJMP) with  ARM-only  */
    if (PASS == CompareMatches(&SM_Arm,&SM_ConnexArm)) cout << "OK ! Arm == Arm-Connex"<<endl<<endl;
    else cout << "Match test has FAILed. Arm and ConnexArm got different results !"<<endl<<endl;
    #endif

    /* STEP6: Compute on CPU with OMP */
    Start = GetMilliCount();
    FindMatchesOMP(&SiftDescriptors1, &SiftDescriptors2, &SM_Arm_OMP,0);
    Delta  = GetMilliSpan(Start);
    cout<<"> cpu-omp FindMatches ran in " << Delta << " ms ("<< BruteMatches/Delta/1000 <<" MM/s)"<<flush<<endl;
    fprintf(logfile, "cpu-omp_ran_in_time %d %f MM/s \n", Delta, BruteMatches/Delta/1000);

    /* STEP7: Compare ARM_OMP with  ARM-only  */
    if (PASS == CompareMatches(&SM_Arm,&SM_Arm_OMP)) cout << "OK ! Arm == Arm-OMP"<<endl<<endl;
    else cout << "Match test has FAILed. Arm and ConnexArm got different results !"<<endl<<endl;

    /* STEP9: Compute on optimized(Red+Calc) Connex-S with JMP  */
    /*
    Start = GetMilliCount();
    connexJmpFindMatchesMt(MODE_CREATE_BATCHES, JMP_BASIC_MATCHING_BNR, &SiftDescriptors1, &SiftDescriptors2, &SM_ConnexArmMan3, &SM_ConnexArm3);
    cout<<"  ConnexS-JMPMt Batches were created in " << GetMilliSpan(Start)<< " ms"<<flush<<endl;

    Start = GetMilliCount();
    connexJmpFindMatchesMt(MODE_EXECUTE_FIND_MATCHES, JMP_BASIC_MATCHING_BNR, &SiftDescriptors1, &SiftDescriptors2, &SM_ConnexArmMan3, &SM_ConnexArm3);
    Delta  = GetMilliSpan(Start);
    cout<<"> ConnexS-JMPMt connexFindMatches ran in " << Delta << " ms ("<< BruteMatches/Delta/1000 <<" MM/s)"<<flush<<endl;

    // STEP7: Compare optimized(Red+Calc) Connex-S with JMP against ARM-only
    if (PASS == CompareMatches(&SM_Arm,&SM_ConnexArm3)) cout << "OK! Arm == JMP Arm-Connex "<<endl<<endl;
    else cout << "Match test has FAILed. Arm and JMP ConnexArm got different results !"<<endl<<endl;
    */

    //IntProofConcept();
    //cout<< "IBC = "<<cnxvector::dwInBatchCounter[0]<<endl;
    //if (PASS == VERIFY_BATCH(0)) cout << "Verification is PASS"<<endl;
    //else cout << "Verification is FAIL"<<endl;
    //if (PASS == DEASM_BATCH(0)) cout << "Verification is PASS"<<endl;
    //else cout << "Verification is FAIL"<<endl;

    //TestBatch();
    //DEASM_BATCH(0);
    //PrintMatches(&SM);
    //PrintDescriptors(&SiftDescriptors1);
/*
    if (testFails ==0)
        cout<<endl<< " All SimpleTests PASSED." <<endl;
    else
        cout<< testFails << " SimpleTests failed." <<endl;
    return testFails;
*/
//    free(BasicMatchRedResults);
	return 0;
}

