
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

#define MAX_DESCRIPTORS 5000
#define MAX_MATCHES (100*1000)
#define MAX_REDUCES (8*1024* 1024) // can hold 8MiB reductions

struct SiftDescriptors
{
    SiftDescriptor SD[MAX_DESCRIPTORS];
    UINT16 SiftDescriptorsBasicFeatures[MAX_DESCRIPTORS][FEATURES_PER_DESCRIPTOR];
    UINT16 RealDescriptors;
};

struct SiftMatch
{
    float X1;
    float Y1;
    float X2;
    float Y2;
    UINT32 Score;
};

struct SiftMatches
{
    SiftMatch Matches[MAX_MATCHES];
    UINT16 RealMatches;
};

SiftDescriptors SiftDescriptors1;
SiftDescriptors SiftDescriptors2;
SiftMatches SM_Arm;
SiftMatches SM_ConnexArm;
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
        printf("Matches[%d].X1,Y1,X2,Y2,Sc= %f %f %f %f %d\n",
               MatchesIndex, SMs->Matches[MatchesIndex].X1, SMs->Matches[MatchesIndex].Y1,
               SMs->Matches[MatchesIndex].X2,SMs->Matches[MatchesIndex].Y2, SMs->Matches[MatchesIndex].Score);
    }
}

static int CompareMatches(SiftMatches *SMs1, SiftMatches *SMs2)
{
    int CntMax = SMs1->RealMatches;
    cout << "Comparing "<<CntMax<< " matches... "<<endl;
    if (CntMax != SMs2->RealMatches) return FAIL;
    for (int cnt = 0; cnt < CntMax; cnt++)
        if ((SMs1->Matches[cnt].Score != SMs2->Matches[cnt].Score) ||
            (SMs1->Matches[cnt].X1 != SMs2->Matches[cnt].X1) ||
            (SMs1->Matches[cnt].X2 != SMs2->Matches[cnt].X2) ||
            (SMs1->Matches[cnt].Y1 != SMs2->Matches[cnt].Y1) ||
            (SMs1->Matches[cnt].Y2 != SMs2->Matches[cnt].Y2)
            )
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

void FindMatches(SiftDescriptors *SDs1, SiftDescriptors *SDs2)
{
/*
    if (SDs1->RealDescriptors > SDs2->RealDescriptors)
    {
        SiftDescriptors *SDsman = SDs1;
        SDs1 = SDs2;
        SDs2 = SDsman;
    }
*/
	//for (int DescriptorIndex1 =0; DescriptorIndex1 < SDs1->RealDescriptors; DescriptorIndex1++)
	for (int DescriptorIndex1 =0; DescriptorIndex1 < 1; DescriptorIndex1++)
    {
        for (int DescriptorIndex2 =0; DescriptorIndex2 < SDs2->RealDescriptors; DescriptorIndex2++)
        //for (int DescriptorIndex2 =0; DescriptorIndex2 < 10; DescriptorIndex2++)
	    {
            UINT32 Score = 0;
			int FeatIndex;
            for (FeatIndex = 0; FeatIndex < FEATURES_PER_DESCRIPTOR; FeatIndex++)
            {
                INT32 sq = (SDs1->SiftDescriptorsBasicFeatures[DescriptorIndex1][FeatIndex] -
                            SDs2->SiftDescriptorsBasicFeatures[DescriptorIndex2][FeatIndex]);
                Score += sq*sq;
            }
            //cout <<"Sc=["<<DescriptorIndex2<<"]= "<<Score<<" ";
            //if ((DescriptorIndex2 & 3) == 3) cout<<endl;
        }
    }
}


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
        SMs->Matches[SMs->RealMatches].Score = (UINT32)-1;
		int DescriptorIndex2;
        for (DescriptorIndex2 =0; DescriptorIndex2 < SDs2->RealDescriptors; DescriptorIndex2++)
	    {
            UINT32 Score = 0;
			int FeatIndex;
            for (FeatIndex = 0; FeatIndex < FEATURES_PER_DESCRIPTOR; FeatIndex++)
            {
                INT32 sq = (SDs1->SiftDescriptorsBasicFeatures[DescriptorIndex1][FeatIndex] -
                            SDs2->SiftDescriptorsBasicFeatures[DescriptorIndex2][FeatIndex]);
                Score += sq*sq;
            }

            //if (DescriptorIndex1==0)
            //{ cout<<Score<<" "; if ((DescriptorIndex2 % 8) == 0) cout<<endl; }

            if (Score < SMs->Matches[SMs->RealMatches].Score)
            {
                SMs->Matches[SMs->RealMatches].X1 = SDs1->SD[DescriptorIndex1].X;
                SMs->Matches[SMs->RealMatches].Y1 = SDs1->SD[DescriptorIndex1].Y;

                SMs->Matches[SMs->RealMatches].X2 = SDs2->SD[DescriptorIndex2].X;
                SMs->Matches[SMs->RealMatches].Y2 = SDs2->SD[DescriptorIndex2].Y;

                SMs->Matches[SMs->RealMatches].Score = Score;
            }
        }
        SMs->RealMatches++;
    }
}

/*
void FindMatches2T(SiftDescriptors *SDs1, SiftDescriptors *SDs2, SiftMatches* SMs)
{
    pid_t childpid;
    int status;

    int RealMatches;
    int DescriptorIndex1;

    SMs->RealMatches = SDs1->RealDescriptors;

    childpid = fork();
    if (childpid >= 0) //fork success
    {
        if (childpid == 0) DescriptorIndex1 = 1;
        else DescriptorIndex1 = 0;

        for (; DescriptorIndex1 < SDs1->RealDescriptors; DescriptorIndex1+=2)
        {
            SMs->Matches[SMs->RealMatches].Score = (UINT32)-1;
            int DescriptorIndex2;
            for (DescriptorIndex2 =0; DescriptorIndex2 < SDs2->RealDescriptors; DescriptorIndex2++)
            {
                UINT32 Score = 0;
                int FeatIndex;
                for (FeatIndex = 0; FeatIndex < FEATURES_PER_DESCRIPTOR; FeatIndex++)
                {
                    INT32 sq = (SDs1->SiftDescriptorsBasicFeatures[DescriptorIndex1][FeatIndex] -
                                SDs2->SiftDescriptorsBasicFeatures[DescriptorIndex2][FeatIndex]);
                    Score += sq*sq;
                }

                //if (DescriptorIndex1==0)
                //{ cout<<Score<<" "; if ((DescriptorIndex2 % 8) == 0) cout<<endl; }

                if (Score < SMs->Matches[RealMatches].Score)
                {
                    SMs->Matches[RealMatches].X1 = SDs1->SD[DescriptorIndex1].X;
                    SMs->Matches[RealMatches].Y1 = SDs1->SD[DescriptorIndex1].Y;

                    SMs->Matches[RealMatches].X2 = SDs2->SD[DescriptorIndex2].X;
                    SMs->Matches[RealMatches].Y2 = SDs2->SD[DescriptorIndex2].Y;

                    SMs->Matches[RealMatches].Score = Score;
                }
            }
        }
        if (childpid > 0) //parent
            wait(&status);
        else exit(0);//child
    }
}*/

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
    int TimeStart;
    int TotalTime = 0;

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
}

static int connexFindMatchesPass1(int RunningMode,int LoadToRxBatchNumber,
                                    SiftDescriptors *SiftDescriptors1, SiftDescriptors *SiftDescriptors2,
                                        SiftMatches* SMs)
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
                EXECUTE_BATCH(UsingBuffer0or1);
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
                                        SiftMatches* SMs)
{
    if (RunningMode == MODE_CREATE_BATCHES)
    {
        // After running, get reduced results
        //clear scores (set score to the max):
        for (int CntDescIm1 = 0; CntDescIm1 < SiftDescriptors1->RealDescriptors; CntDescIm1++)
            SMs->Matches[CntDescIm1].Score = (UINT32)-1;
    }
    else // (RunningMode == MODE_EXECUTE_FIND_MATCHES)
    {
        int CurrentcnxvectorChunkImg1, CurrentcnxvectorChunkImg2;
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
                            if ((BasicMatchRedResults[RedCounter]) < SMs->Matches[descIm1].Score)
                            {
                                SMs->Matches[descIm1].Score = BasicMatchRedResults[RedCounter];
                                SMs->Matches[descIm1].X1 = SiftDescriptors1->SD[descIm1].X;
                                SMs->Matches[descIm1].Y1 = SiftDescriptors1->SD[descIm1].Y;
                                SMs->Matches[descIm1].X2 = SiftDescriptors2->SD[descIm2].X;
                                SMs->Matches[descIm1].Y2 = SiftDescriptors2->SD[descIm2].Y;
                            }
                            RedCounter++;
                        }
                    }
            }
         }
    }
    SMs->RealMatches = SiftDescriptors1->RealDescriptors;
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

    LoadDescriptors((char*)"data/adam1.key", &SiftDescriptors1, 0);
    LoadDescriptors((char*)"data/adam2.key", &SiftDescriptors2, 0);

    //FindMatches(&SiftDescriptors1, &SiftDescriptors2);
    Start = GetMilliCount();
    FindMatches(&SiftDescriptors1, &SiftDescriptors2, &SM_Arm);
    cout<<"armFindMatches ran in " << GetMilliSpan(Start)<< " ms"<<endl;

    //Start = GetMilliCount();
    //FindMatches2T(&SiftDescriptors1, &SiftDescriptors2, &SM_Arm);
    //cout<<"armFindMatches (2 threads) ran in " << GetMilliSpan(Start)<< " ms"<<endl;

    //PrintMatches(&SM_Arm);
    Start = GetMilliCount();
    connexFindMatchesPass1(MODE_CREATE_BATCHES, BASIC_MATCHING_BNR, &SiftDescriptors1, &SiftDescriptors2, &SM_ConnexArm);
    connexFindMatchesPass2(MODE_CREATE_BATCHES, BASIC_MATCHING_BNR, &SiftDescriptors1, &SiftDescriptors2, &SM_ConnexArm);

    //connexFM_CreateBatch(BASIC_MATCHING_BNR, 0);
    //connexFM_CreateBatch(BASIC_MATCHING_BNR+1, 1);

    cout<<"Batches were created in " << GetMilliSpan(Start)<< " ms"<<endl;

    Start = GetMilliCount();
    connexFindMatchesPass1(MODE_EXECUTE_FIND_MATCHES, BASIC_MATCHING_BNR, &SiftDescriptors1, &SiftDescriptors2, &SM_ConnexArm);
    connexFindMatchesPass2(MODE_EXECUTE_FIND_MATCHES, BASIC_MATCHING_BNR, &SiftDescriptors1, &SiftDescriptors2, &SM_ConnexArm);
    cout<<"connexFindMatches ran in " << GetMilliSpan(Start)<< " ms"<<endl;
    //PrintMatches(&SM_ConnexArm);

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

