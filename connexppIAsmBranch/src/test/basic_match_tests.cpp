
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
producing a �.key� ASCII file. This file has one line per keypoint, with the x and y coordinates (pixels), the scale (pixels),
the orientation (radians) and 128 numbers (in the range 0-255) representing the descriptor.
This file is almost equivalent to the output of D. Lowe�s original implementation, except that x and y are swapped
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
#define MAX_REDUCES (8*1024* 1024)

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

SiftDescriptors SiftDescriptors1;
SiftDescriptors SiftDescriptors2;
SiftMatches SM_Arm;
SiftMatches SM_ConnexArm;
SiftMatches SM_ConnexArmMan;
UINT_RED_REG_VAL BasicMatchRedResults[MAX_REDUCES];

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
        /*
        if ((SMs1->Matches[cnt].Score != SMs2->Matches[cnt].Score) ||
            (SMs1->Matches[cnt].X1 != SMs2->Matches[cnt].X1) ||
            (SMs1->Matches[cnt].X2 != SMs2->Matches[cnt].X2) ||
            (SMs1->Matches[cnt].Y1 != SMs2->Matches[cnt].Y1) ||
            (SMs1->Matches[cnt].Y2 != SMs2->Matches[cnt].Y2)
            )
        */
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
void FindMatches(SiftDescriptors *SDs1, SiftDescriptors *SDs2, SiftMatches* SMs)
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

			int FeatIndex;
            for (FeatIndex = 0; FeatIndex < FEATURES_PER_DESCRIPTOR; FeatIndex++)
            {
                INT32 sq = (SDs1->SiftDescriptorsBasicFeatures[DescriptorIndex1][FeatIndex] -
                            SDs2->SiftDescriptorsBasicFeatures[DescriptorIndex2][FeatIndex]);
                dsq += sq*sq;
            }

            //if (DescriptorIndex1==0)
            //{ cout<<LastScore<<" "; if ((DescriptorIndex2 % 8) == 0) cout<<endl; }

            //BestScore = LastScore;
            //SMs->DescriptorIndexInSecondImage[SMs->RealMatches] = DescriptorIndex2;
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

io_unit IOU_CVCI1;
io_unit IOU_CVCI2;
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

int connexFindMatchesPass2(int RunningMode,int LoadToRxBatchNumber,
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
#define LOAD_VECTOR(VECT_ADDRESS, MEMORY_ADDRESS) \

#define FOR_ALL(Items) {for(int Items = 0; Items <
#define ITEMS
#define IN
//#define

//FORALL(x,_OF_TYPE,VECTORS_CHUNK_IMAGE1,_IN, SiftDescriptors1->RealDescriptors)

//#define FORALL(items,_OF_SIZE(sz)_IN(collection))
//for (int x = 0; x < (SiftDescriptors1->RealDescriptors + VECTORS_CHUNK_IMAGE1 -1)/VECTORS_CHUNK_IMAGE1; x++)

//for (int x = 0; x < (SiftDescriptors1->RealDescriptors + VECTORS_CHUNK_IMAGE1 -1)/VECTORS_CHUNK_IMAGE1; x++)
//forall(SiftDescriptorsIn(SiftDescriptors1))

//void forall(int VECTORS_CHUNK_IMAGE1, int SiftDescriptors1->RealDescriptors)

#define PRESTORE_VECTORS(IO_OBJ,CNXVECT_ADDRESS, MEM_ADDRESS)\
    IO_OBJ.preWritecnxvectors(CNXVECT_ADDRESS,\
        SiftDescriptors1->SiftDescriptorsBasicFeatures[VECTORS_CHUNK_IMAGE1*CurrentcnxvectorChunkImg1],\
        VECTORS_CHUNK_IMAGE1);

#define LOAD_VECTORS_NOW(IO_OBJ) \
if (PASS != IO_WRITE_NOW(&IO_OBJ)) {printf("Writing next object to IO pipe, FAILED !"); return FAIL;}

static void TestBatch()
{
    int x;
    BEGIN_BATCH(2);
    for(x = 0; x < 30; x++)
                        {
                            R31 = R30 - R[x];
                            R31 = R31 * R31;
                            REDUCE(R31);
                            cout<< "R["<< x <<"]"<<endl;
                        }
    END_BATCH();
    DEASM_BATCH(2);
}

#define BASIC_MATCHING_BNR 0
int test_BasicMatching_All()
{
    //forcing descriptors to have proper size: multiple of 364 for 1, multiple of 330 for second
    //const int MAX_IMG_1_DECRIPTORS = 364*5;//max 2306
    //const int MAX_IMG_2_DECRIPTORS = 330*3;//max 1196
    int Start;

    //LoadDescriptors((char*)"data/adam1.key", &SiftDescriptors1, MAX_IMG_1_DECRIPTORS);
    //LoadDescriptors((char*)"data/adam2.key", &SiftDescriptors2, MAX_IMG_2_DECRIPTORS);

    //LoadDescriptors((char*)"data/adam1_big.png.key", &SiftDescriptors1, 0);
    //LoadDescriptors((char*)"data/adam2_big.png.key", &SiftDescriptors2, 0);

    LoadDescriptors((char*)"data/img1.png.key", &SiftDescriptors1, 0);
    LoadDescriptors((char*)"data/img3.png.key", &SiftDescriptors2, 0);

    //LoadDescriptors((char*)"data/adam1_big_siftpp.key", &SiftDescriptors1, 0);
    //LoadDescriptors((char*)"data/adam2_big_siftpp.key", &SiftDescriptors2, 0);

    //LoadDescriptors((char*)"data/img3_siftpp.key", &SiftDescriptors1, 0);
    //LoadDescriptors((char*)"data/img3_siftpp.key", &SiftDescriptors2, 0);
    //LoadDescriptors((char*)"data/img1_siftpp.key", &SiftDescriptors1, 0);
    //LoadDescriptors((char*)"data/img3_siftpp.key", &SiftDescriptors2, 0);

    //Start = GetMilliCount();
    //FindMatches2T(&SiftDescriptors1, &SiftDescriptors2, &SM_Arm);
    //cout<<"armFindMatches (2 threads) ran in " << GetMilliSpan(Start)<< " ms"<<endl;

    //PrintMatches(&SM_Arm);
    Start = GetMilliCount();
    connexFindMatchesPass1(MODE_CREATE_BATCHES, BASIC_MATCHING_BNR, &SiftDescriptors1, &SiftDescriptors2);
    connexFindMatchesPass2(MODE_CREATE_BATCHES, BASIC_MATCHING_BNR, &SiftDescriptors1, &SiftDescriptors2, &SM_ConnexArmMan, &SM_ConnexArm);
    cout<<"Batches were created in " << GetMilliSpan(Start)<< " ms"<<flush<<endl;

    //database
    /*
    Start = GetMilliCount();
    if (PASS != kernel_acc::storeKernel("database/BasicMatchingA.ker", BASIC_MATCHING_BNR))
        cout<<"Could not store kernel "<<endl;

    if (PASS != kernel_acc::storeKernel("database/BasicMatchingB.ker", BASIC_MATCHING_BNR+1))
        cout<<"Could not store kernel "<<endl;

    cout<<"Kernels were stored in " << GetMilliSpan(Start)<< " ms"<<endl;
    */
    //connexFM_CreateBatch(BASIC_MATCHING_BNR, 0);
    //connexFM_CreateBatch(BASIC_MATCHING_BNR+1, 1);
    /*
    Start = GetMilliCount();
    if (PASS != kernel_acc::loadKernel((char*)"database/BasicMatchingA.ker", BASIC_MATCHING_BNR+2))
        cout<<"Could not load kernel "<<endl;
    if (PASS != kernel_acc::loadKernel("database/BasicMatchingB.ker", BASIC_MATCHING_BNR+3))
        cout<<"Could not load kernel "<<endl;

    cout<<"Batches were loaded in " << GetMilliSpan(Start)<< " ms"<<endl;
    //cout<<"Could not load batches "<<endl;
    */

    Start = GetMilliCount();
    connexFindMatchesPass1(MODE_EXECUTE_FIND_MATCHES, BASIC_MATCHING_BNR, &SiftDescriptors1, &SiftDescriptors2);
    connexFindMatchesPass2(MODE_EXECUTE_FIND_MATCHES, BASIC_MATCHING_BNR, &SiftDescriptors1, &SiftDescriptors2, &SM_ConnexArmMan, &SM_ConnexArm);
    cout<<"connexFindMatches ran in " << GetMilliSpan(Start)<< " ms"<<flush<<endl;
    //PrintMatches(&SM_ConnexArm);

    Start = GetMilliCount();
    FindMatches(&SiftDescriptors1, &SiftDescriptors2, &SM_Arm);
    cout<<"armFindMatches ran in " << GetMilliSpan(Start)<< " ms"<<flush<<endl;

    if (PASS == CompareMatches(&SM_Arm,&SM_ConnexArm)) cout << "Matches are a ... match ;). Arm and Arm-Connex got the same results."<<endl;
    else cout << "Match test has FAILed. Arm and ConnexArm got different results !"<<endl;

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
	return 0;
}

