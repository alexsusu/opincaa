/* =============================================================================
 * Name:     array.cpp
 * Purpose:  Code for "Vector-C Programmer Manual", section "Examples".
 *           Parallel code running on the array.
 * Created:  28APR2010
 * Version:  1.01
 * Author:   Bogdan Mitu
 * ============================================================================
 * Copyright (C) 2010 by Allsearch Semi, LLC.  All rights reserved.
 */

#include <vector.h>
#include <connex.h>
#include <graphics.h>
#include <stdio.h>

#define FRAME_HEIGHT 128
#define FRAME_WIDTH 128
/*
 * Apply low-pass filter (0.25 0.5 0.25) horizontally and vertically
 */
 
short* inBuffer;
short* outBuffer;

void applyFilterThread(){
  vector *outputFrame = &localMemory[0];
  vector *inputFrame = &localMemory[FRAME_HEIGHT];

  int i;

  register vector _tmp1;
  register vector _tmp2;

  read(inputFrame, FRAME_HEIGHT, inBuffer);

  for (i = 1; i < FRAME_HEIGHT- 1; i++)
  {
    /* apply filter vertically */
    outputFrame[i] = bitRightShift(inputFrame[i-1] + inputFrame[i] * 2 + inputFrame[i+1], 2);

	/* apply filter horizontally */

	_tmp1 = shiftLeft(&outputFrame[i], 1, REPEAT);
	_tmp2 = shiftRight(&outputFrame[i], 1, REPEAT);
    outputFrame[i] = bitRightShift(_tmp1 + bitLeftShift(outputFrame[i], 1) + _tmp2, 2);
	
		
  }
  /* Special cases: 1st and last line */
	outputFrame[0] = bitRightShift(bitLeftShift(inputFrame[0], 2) + inputFrame[0] + inputFrame[1], 2);

	_tmp1 = shiftLeft(&outputFrame[0], 1, REPEAT);
	_tmp2 = shiftRight(&outputFrame[0], 1, REPEAT);
	outputFrame[0] = bitRightShift(_tmp1 + bitLeftShift(outputFrame[0], 1) + _tmp2, 2);

  
	outputFrame[FRAME_HEIGHT-1] = bitRightShift(inputFrame[FRAME_HEIGHT-2] + bitLeftShift(inputFrame[FRAME_HEIGHT-1], 1) + inputFrame[FRAME_HEIGHT-1], 2);

	_tmp1 = shiftLeft(&outputFrame[FRAME_HEIGHT-1], 1, REPEAT);
	_tmp2 = shiftRight(&outputFrame[FRAME_HEIGHT-1], 1, REPEAT);
	outputFrame[FRAME_HEIGHT-1] = bitRightShift(_tmp1 + bitLeftShift(outputFrame[FRAME_HEIGHT-1],1) + _tmp2, 2);
    
  write(outputFrame, FRAME_HEIGHT, outBuffer);
//	write(inputFrame, FRAME_HEIGHT, outBuffer);
}

void applyFilter(){
	applyFilterThread();
}


int main(){
	int i;
	inBuffer = (short*)0x200000;
	outBuffer = (short*)0x400000;
	applyFilter();
	initGraphics();
	put_image(0, 0, 128, 128, inBuffer);
	put_image(128, 0, 128, 128, outBuffer);
	//line(0,200,800,200,WHITE);
//	for(i=0; i<800; i++){
//		putpixel(i,200,WHITE);
//	}
	return 0;
}

void user_swi_handler(){}
void user_irq_handler(){}

#include <__main.c>
