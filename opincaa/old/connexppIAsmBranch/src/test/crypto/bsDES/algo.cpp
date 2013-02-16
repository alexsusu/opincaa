
/*
 * Bitslice implementation of DES.
 *
 * Checks that the plaintext bits p[0] .. p[63]
 * encrypt to the ciphertext bits c[0] .. c[63]
 * given the key bits k[0] .. k[55]
 */

 #include "../../../../include/test/crypto/bsDES/sboxes.h"

	UINTX	l0 ;
	UINTX	l1 ;
	UINTX	l2 ;
	UINTX	l3 ;
	UINTX	l4 ;
	UINTX	l5 ;
	UINTX	l6 ;
	UINTX	l7 ;
	UINTX	l8 ;
	UINTX	l9 ;
	UINTX	l10;
	UINTX	l11;
	UINTX	l12;
	UINTX	l13;
	UINTX	l14;
	UINTX	l15;
	UINTX	l16;
	UINTX	l17;
	UINTX	l18;
	UINTX	l19;
	UINTX	l20;
	UINTX	l21;
	UINTX	l22;
	UINTX	l23;
	UINTX	l24;
	UINTX	l25;
	UINTX	l26;
	UINTX	l27;
	UINTX	l28;
	UINTX	l29;
	UINTX	l30;
	UINTX	l31;
	UINTX	r0 ;
	UINTX	r1 ;
	UINTX	r2 ;
	UINTX	r3 ;
	UINTX	r4 ;
	UINTX	r5 ;
	UINTX	r6 ;
	UINTX	r7 ;
	UINTX	r8 ;
	UINTX	r9 ;
	UINTX	r10 ;
	UINTX	r11 ;
	UINTX	r12 ;
	UINTX	r13 ;
	UINTX	r14 ;
	UINTX	r15 ;
	UINTX	r16 ;
	UINTX	r17 ;
	UINTX	r18 ;
	UINTX	r19 ;
	UINTX	r20 ;
	UINTX	r21 ;
	UINTX	r22 ;
	UINTX	r23 ;
	UINTX	r24 ;
	UINTX	r25 ;
	UINTX	r26 ;
	UINTX	r27 ;
	UINTX	r28 ;
	UINTX	r29 ;
	UINTX	r30 ;
	UINTX	r31 ;

/*
p is pointer to UINTX[64]
k is pointer to UINTX[56]
*/
void desencrypt(UINTX *p,UINTX *k)
{
    int i;
	l0 = p[6];
	l1 = p[14];
	l2 = p[22];
	l3 = p[30];
	l4 = p[38];
	l5 = p[46];
	l6 = p[54];
	l7 = p[62];
	l8 = p[4];
	l9 = p[12];
	l10 = p[20];
	l11 = p[28];
	l12 = p[36];
	l13 = p[44];
	l14 = p[52];
	l15 = p[60];
	l16 = p[2];
	l17 = p[10];
	l18 = p[18];
	l19 = p[26];
	l20 = p[34];
	l21 = p[42];
	l22 = p[50];
	l23 = p[58];
	l24 = p[0];
	l25 = p[8];
	l26 = p[16];
	l27 = p[24];
	l28 = p[32];
	l29 = p[40];
	l30 = p[48];
	l31 = p[56];
	r0 = p[7];
	r1 = p[15];
	r2 = p[23];
	r3 = p[31];
	r4 = p[39];
	r5 = p[47];
	r6 = p[55];
	r7 = p[63];
	r8 = p[5];
	r9 = p[13];
	r10 = p[21];
	r11 = p[29];
	r12 = p[37];
	r13 = p[45];
	r14 = p[53];
	r15 = p[61];
	r16 = p[3];
	r17 = p[11];
	r18 = p[19];
	r19 = p[27];
	r20 = p[35];
	r21 = p[43];
	r22 = p[51];
	r23 = p[59];
	r24 = p[1];
	r25 = p[9];
	r26 = p[17];
	r27 = p[25];
	r28 = p[33];
	r29 = p[41];
	r30 = p[49];
	r31 = p[57];


	//for (i=0; i<64;i++) printf("%d\n",p[i]);

	s1 (r31 ^ k[47], r0 ^ k[11], r1 ^ k[26], r2 ^ k[3], r3 ^ k[13],
		r4 ^ k[41], &l8, &l16, &l22, &l30);

	s2 (r3 ^ k[27], r4 ^ k[6], r5 ^ k[54], r6 ^ k[48], r7 ^ k[39],
		r8 ^ k[19], &l12, &l27, &l1, &l17);
	s3 (r7 ^ k[53], r8 ^ k[25], r9 ^ k[33], r10 ^ k[34], r11 ^ k[17],
		r12 ^ k[5], &l23, &l15, &l29, &l5);
	s4 (r11 ^ k[4], r12 ^ k[55], r13 ^ k[24], r14 ^ k[32], r15 ^ k[40],
		r16 ^ k[20], &l25, &l19, &l9, &l0);
	s5 (r15 ^ k[36], r16 ^ k[31], r17 ^ k[21], r18 ^ k[8], r19 ^ k[23],
		r20 ^ k[52], &l7, &l13, &l24, &l2);
	s6 (r19 ^ k[14], r20 ^ k[29], r21 ^ k[51], r22 ^ k[9], r23 ^ k[35],
		r24 ^ k[30], &l3, &l28, &l10, &l18);
	s7 (r23 ^ k[2], r24 ^ k[37], r25 ^ k[22], r26 ^ k[0], r27 ^ k[42],
		r28 ^ k[38], &l31, &l11, &l21, &l6);
	s8 (r27 ^ k[16], r28 ^ k[43], r29 ^ k[44], r30 ^ k[1], r31 ^ k[7],
		r0 ^ k[28], &l4, &l26, &l14, &l20);

//    for (i=0; i < 56; i++)
  //      printf("%d",k[55-i] & 0x01);
    printf("\n Output 1: ");

    printf("%d",l31 & 0x01); printf("%d",l30 & 0x01); printf("%d",l29 & 0x01); printf("%d",l28 & 0x01);
    printf("%d",l27 & 0x01);printf("%d",l26 & 0x01);printf("%d",l25 & 0x01);printf("%d ",l24 & 0x01);
    printf("%d",l23 & 0x01);printf("%d",l22 & 0x01);printf("%d",l21 & 0x01);printf("%d",l20 & 0x01);
    printf("%d",l19 & 0x01);printf("%d",l18 & 0x01);printf("%d",l17 & 0x01);printf("%d ",l16 & 0x01);
    printf("%d",l15 & 0x01);printf("%d",l14 & 0x01);printf("%d",l13 & 0x01);printf("%d",l12 & 0x01);
    printf("%d",l11 & 0x01);printf("%d",l10 & 0x01);printf("%d",l9 & 0x01);printf("%d ",l8 & 0x01);
    printf("%d",l7 & 0x01);printf("%d",l6 & 0x01);printf("%d",l5 & 0x01);printf("%d",l4 & 0x01);
    printf("%d",l3 & 0x01);printf("%d",l2 & 0x01);printf("%d",l1 & 0x01);printf("%d ",l0 & 0x01);

    printf("\n");

	s1 (l31 ^ k[54], l0 ^ k[18], l1 ^ k[33], l2 ^ k[10], l3 ^ k[20],
		l4 ^ k[48], &r8, &r16, &r22, &r30);
	s2 (l3 ^ k[34], l4 ^ k[13], l5 ^ k[4], l6 ^ k[55], l7 ^ k[46],
		l8 ^ k[26], &r12, &r27, &r1, &r17);
	s3 (l7 ^ k[3], l8 ^ k[32], l9 ^ k[40], l10 ^ k[41], l11 ^ k[24],
		l12 ^ k[12], &r23, &r15, &r29, &r5);
	s4 (l11 ^ k[11], l12 ^ k[5], l13 ^ k[6], l14 ^ k[39], l15 ^ k[47],
		l16 ^ k[27], &r25, &r19, &r9, &r0);
	s5 (l15 ^ k[43], l16 ^ k[38], l17 ^ k[28], l18 ^ k[15], l19 ^ k[30],
		l20 ^ k[0], &r7, &r13, &r24, &r2);
	s6 (l19 ^ k[21], l20 ^ k[36], l21 ^ k[31], l22 ^ k[16], l23 ^ k[42],
		l24 ^ k[37], &r3, &r28, &r10, &r18);
	s7 (l23 ^ k[9], l24 ^ k[44], l25 ^ k[29], l26 ^ k[7], l27 ^ k[49],
		l28 ^ k[45], &r31, &r11, &r21, &r6);
	s8 (l27 ^ k[23], l28 ^ k[50], l29 ^ k[51], l30 ^ k[8], l31 ^ k[14],
		l0 ^ k[35], &r4, &r26, &r14, &r20);

    printf("\n Output 2: ");

    printf("%d",r31 & 0x01); printf("%d",r30 & 0x01); printf("%d",r29 & 0x01); printf("%d",r28 & 0x01);
    printf("%d",r27 & 0x01);printf("%d",r26 & 0x01);printf("%d",r25 & 0x01);printf("%d ",r24 & 0x01);
    printf("%d",r23 & 0x01);printf("%d",r22 & 0x01);printf("%d",r21 & 0x01);printf("%d",r20 & 0x01);
    printf("%d",r19 & 0x01);printf("%d",r18 & 0x01);printf("%d",r17 & 0x01);printf("%d ",r16 & 0x01);
    printf("%d",r15 & 0x01);printf("%d",r14 & 0x01);printf("%d",r13 & 0x01);printf("%d",r12 & 0x01);
    printf("%d",r11 & 0x01);printf("%d",r10 & 0x01);printf("%d",r9 & 0x01);printf("%d ",r8 & 0x01);
    printf("%d",r7 & 0x01);printf("%d",r6 & 0x01);printf("%d",r5 & 0x01);printf("%d",r4 & 0x01);
    printf("%d",r3 & 0x01);printf("%d",r2 & 0x01);printf("%d",r1 & 0x01);printf("%d ",r0 & 0x01);

    printf("\n");

	s1 (r31 ^ k[11], r0 ^ k[32], r1 ^ k[47], r2 ^ k[24], r3 ^ k[34],
		r4 ^ k[5], &l8, &l16, &l22, &l30);
	s2 (r3 ^ k[48], r4 ^ k[27], r5 ^ k[18], r6 ^ k[12], r7 ^ k[3],
		r8 ^ k[40], &l12, &l27, &l1, &l17);
	s3 (r7 ^ k[17], r8 ^ k[46], r9 ^ k[54], r10 ^ k[55], r11 ^ k[13],
		r12 ^ k[26], &l23, &l15, &l29, &l5);
	s4 (r11 ^ k[25], r12 ^ k[19], r13 ^ k[20], r14 ^ k[53], r15 ^ k[4],
		r16 ^ k[41], &l25, &l19, &l9, &l0);
	s5 (r15 ^ k[2], r16 ^ k[52], r17 ^ k[42], r18 ^ k[29], r19 ^ k[44],
		r20 ^ k[14], &l7, &l13, &l24, &l2);
	s6 (r19 ^ k[35], r20 ^ k[50], r21 ^ k[45], r22 ^ k[30], r23 ^ k[1],
		r24 ^ k[51], &l3, &l28, &l10, &l18);
	s7 (r23 ^ k[23], r24 ^ k[31], r25 ^ k[43], r26 ^ k[21], r27 ^ k[8],
		r28 ^ k[0], &l31, &l11, &l21, &l6);
	s8 (r27 ^ k[37], r28 ^ k[9], r29 ^ k[38], r30 ^ k[22], r31 ^ k[28],
		r0 ^ k[49], &l4, &l26, &l14, &l20);

    printf("\n Output 3: ");

    printf("%d",l31 & 0x01); printf("%d",l30 & 0x01); printf("%d",l29 & 0x01); printf("%d",l28 & 0x01);
    printf("%d",l27 & 0x01);printf("%d",l26 & 0x01);printf("%d",l25 & 0x01);printf("%d ",l24 & 0x01);
    printf("%d",l23 & 0x01);printf("%d",l22 & 0x01);printf("%d",l21 & 0x01);printf("%d",l20 & 0x01);
    printf("%d",l19 & 0x01);printf("%d",l18 & 0x01);printf("%d",l17 & 0x01);printf("%d ",l16 & 0x01);
    printf("%d",l15 & 0x01);printf("%d",l14 & 0x01);printf("%d",l13 & 0x01);printf("%d",l12 & 0x01);
    printf("%d",l11 & 0x01);printf("%d",l10 & 0x01);printf("%d",l9 & 0x01);printf("%d ",l8 & 0x01);
    printf("%d",l7 & 0x01);printf("%d",l6 & 0x01);printf("%d",l5 & 0x01);printf("%d",l4 & 0x01);
    printf("%d",l3 & 0x01);printf("%d",l2 & 0x01);printf("%d",l1 & 0x01);printf("%d ",l0 & 0x01);

    printf("\n");


	s1 (l31 ^ k[25], l0 ^ k[46], l1 ^ k[4], l2 ^ k[13], l3 ^ k[48],
		l4 ^ k[19], &r8, &r16, &r22, &r30);
	s2 (l3 ^ k[5], l4 ^ k[41], l5 ^ k[32], l6 ^ k[26], l7 ^ k[17],
		l8 ^ k[54], &r12, &r27, &r1, &r17);
	s3 (l7 ^ k[6], l8 ^ k[3], l9 ^ k[11], l10 ^ k[12], l11 ^ k[27],
		l12 ^ k[40], &r23, &r15, &r29, &r5);
	s4 (l11 ^ k[39], l12 ^ k[33], l13 ^ k[34], l14 ^ k[10], l15 ^ k[18],
		l16 ^ k[55], &r25, &r19, &r9, &r0);
	s5 (l15 ^ k[16], l16 ^ k[7], l17 ^ k[1], l18 ^ k[43], l19 ^ k[31],
		l20 ^ k[28], &r7, &r13, &r24, &r2);
	s6 (l19 ^ k[49], l20 ^ k[9], l21 ^ k[0], l22 ^ k[44], l23 ^ k[15],
		l24 ^ k[38], &r3, &r28, &r10, &r18);
	s7 (l23 ^ k[37], l24 ^ k[45], l25 ^ k[2], l26 ^ k[35], l27 ^ k[22],
		l28 ^ k[14], &r31, &r11, &r21, &r6);
	s8 (l27 ^ k[51], l28 ^ k[23], l29 ^ k[52], l30 ^ k[36], l31 ^ k[42],
		l0 ^ k[8], &r4, &r26, &r14, &r20);

    printf("\n Output 4: ");

    printf("%d",r31 & 0x01); printf("%d",r30 & 0x01); printf("%d",r29 & 0x01); printf("%d",r28 & 0x01);
    printf("%d",r27 & 0x01);printf("%d",r26 & 0x01);printf("%d",r25 & 0x01);printf("%d ",r24 & 0x01);
    printf("%d",r23 & 0x01);printf("%d",r22 & 0x01);printf("%d",r21 & 0x01);printf("%d",r20 & 0x01);
    printf("%d",r19 & 0x01);printf("%d",r18 & 0x01);printf("%d",r17 & 0x01);printf("%d ",r16 & 0x01);
    printf("%d",r15 & 0x01);printf("%d",r14 & 0x01);printf("%d",r13 & 0x01);printf("%d",r12 & 0x01);
    printf("%d",r11 & 0x01);printf("%d",r10 & 0x01);printf("%d",r9 & 0x01);printf("%d ",r8 & 0x01);
    printf("%d",r7 & 0x01);printf("%d",r6 & 0x01);printf("%d",r5 & 0x01);printf("%d",r4 & 0x01);
    printf("%d",r3 & 0x01);printf("%d",r2 & 0x01);printf("%d",r1 & 0x01);printf("%d ",r0 & 0x01);

    printf("\n");

	s1 (r31 ^ k[39], r0 ^ k[3], r1 ^ k[18], r2 ^ k[27], r3 ^ k[5],
		r4 ^ k[33], &l8, &l16, &l22, &l30);

	s2 (r3 ^ k[19], r4 ^ k[55], r5 ^ k[46], r6 ^ k[40], r7 ^ k[6],
		r8 ^ k[11], &l12, &l27, &l1, &l17);
	s3 (r7 ^ k[20], r8 ^ k[17], r9 ^ k[25], r10 ^ k[26], r11 ^ k[41],
		r12 ^ k[54], &l23, &l15, &l29, &l5);
	s4 (r11 ^ k[53], r12 ^ k[47], r13 ^ k[48], r14 ^ k[24], r15 ^ k[32],
		r16 ^ k[12], &l25, &l19, &l9, &l0);
	s5 (r15 ^ k[30], r16 ^ k[21], r17 ^ k[15], r18 ^ k[2], r19 ^ k[45],
		r20 ^ k[42], &l7, &l13, &l24, &l2);
	s6 (r19 ^ k[8], r20 ^ k[23], r21 ^ k[14], r22 ^ k[31], r23 ^ k[29],
		r24 ^ k[52], &l3, &l28, &l10, &l18);
	s7 (r23 ^ k[51], r24 ^ k[0], r25 ^ k[16], r26 ^ k[49], r27 ^ k[36],
		r28 ^ k[28], &l31, &l11, &l21, &l6);
	s8 (r27 ^ k[38], r28 ^ k[37], r29 ^ k[7], r30 ^ k[50], r31 ^ k[1],
		r0 ^ k[22], &l4, &l26, &l14, &l20);

    printf("\n Output 5: ");

    printf("%d",l31 & 0x01); printf("%d",l30 & 0x01); printf("%d",l29 & 0x01); printf("%d",l28 & 0x01);
    printf("%d",l27 & 0x01);printf("%d",l26 & 0x01);printf("%d",l25 & 0x01);printf("%d ",l24 & 0x01);
    printf("%d",l23 & 0x01);printf("%d",l22 & 0x01);printf("%d",l21 & 0x01);printf("%d",l20 & 0x01);
    printf("%d",l19 & 0x01);printf("%d",l18 & 0x01);printf("%d",l17 & 0x01);printf("%d ",l16 & 0x01);
    printf("%d",l15 & 0x01);printf("%d",l14 & 0x01);printf("%d",l13 & 0x01);printf("%d",l12 & 0x01);
    printf("%d",l11 & 0x01);printf("%d",l10 & 0x01);printf("%d",l9 & 0x01);printf("%d ",l8 & 0x01);
    printf("%d",l7 & 0x01);printf("%d",l6 & 0x01);printf("%d",l5 & 0x01);printf("%d",l4 & 0x01);
    printf("%d",l3 & 0x01);printf("%d",l2 & 0x01);printf("%d",l1 & 0x01);printf("%d ",l0 & 0x01);


	s1 (l31 ^ k[53], l0 ^ k[17], l1 ^ k[32], l2 ^ k[41], l3 ^ k[19],
		l4 ^ k[47], &r8, &r16, &r22, &r30);
	s2 (l3 ^ k[33], l4 ^ k[12], l5 ^ k[3], l6 ^ k[54], l7 ^ k[20],
		l8 ^ k[25], &r12, &r27, &r1, &r17);
	s3 (l7 ^ k[34], l8 ^ k[6], l9 ^ k[39], l10 ^ k[40], l11 ^ k[55],
		l12 ^ k[11], &r23, &r15, &r29, &r5);
	s4 (l11 ^ k[10], l12 ^ k[4], l13 ^ k[5], l14 ^ k[13], l15 ^ k[46],
		l16 ^ k[26], &r25, &r19, &r9, &r0);
	s5 (l15 ^ k[44], l16 ^ k[35], l17 ^ k[29], l18 ^ k[16], l19 ^ k[0],
		l20 ^ k[1], &r7, &r13, &r24, &r2);
	s6 (l19 ^ k[22], l20 ^ k[37], l21 ^ k[28], l22 ^ k[45], l23 ^ k[43],
		l24 ^ k[7], &r3, &r28, &r10, &r18);
	s7 (l23 ^ k[38], l24 ^ k[14], l25 ^ k[30], l26 ^ k[8], l27 ^ k[50],
		l28 ^ k[42], &r31, &r11, &r21, &r6);
	s8 (l27 ^ k[52], l28 ^ k[51], l29 ^ k[21], l30 ^ k[9], l31 ^ k[15],
		l0 ^ k[36], &r4, &r26, &r14, &r20);

    printf("\n Output 6: ");

    printf("%d",r31 & 0x01); printf("%d",r30 & 0x01); printf("%d",r29 & 0x01); printf("%d",r28 & 0x01);
    printf("%d",r27 & 0x01);printf("%d",r26 & 0x01);printf("%d",r25 & 0x01);printf("%d ",r24 & 0x01);
    printf("%d",r23 & 0x01);printf("%d",r22 & 0x01);printf("%d",r21 & 0x01);printf("%d",r20 & 0x01);
    printf("%d",r19 & 0x01);printf("%d",r18 & 0x01);printf("%d",r17 & 0x01);printf("%d ",r16 & 0x01);
    printf("%d",r15 & 0x01);printf("%d",r14 & 0x01);printf("%d",r13 & 0x01);printf("%d",r12 & 0x01);
    printf("%d",r11 & 0x01);printf("%d",r10 & 0x01);printf("%d",r9 & 0x01);printf("%d ",r8 & 0x01);
    printf("%d",r7 & 0x01);printf("%d",r6 & 0x01);printf("%d",r5 & 0x01);printf("%d",r4 & 0x01);
    printf("%d",r3 & 0x01);printf("%d",r2 & 0x01);printf("%d",r1 & 0x01);printf("%d ",r0 & 0x01);

    printf("\n");






	s1 (r31 ^ k[10], r0 ^ k[6], r1 ^ k[46], r2 ^ k[55], r3 ^ k[33],
		r4 ^ k[4], &l8, &l16, &l22, &l30);
	s2 (r3 ^ k[47], r4 ^ k[26], r5 ^ k[17], r6 ^ k[11], r7 ^ k[34],
		r8 ^ k[39], &l12, &l27, &l1, &l17);
	s3 (r7 ^ k[48], r8 ^ k[20], r9 ^ k[53], r10 ^ k[54], r11 ^ k[12],
		r12 ^ k[25], &l23, &l15, &l29, &l5);
	s4 (r11 ^ k[24], r12 ^ k[18], r13 ^ k[19], r14 ^ k[27], r15 ^ k[3],
		r16 ^ k[40], &l25, &l19, &l9, &l0);
	s5 (r15 ^ k[31], r16 ^ k[49], r17 ^ k[43], r18 ^ k[30], r19 ^ k[14],
		r20 ^ k[15], &l7, &l13, &l24, &l2);
	s6 (r19 ^ k[36], r20 ^ k[51], r21 ^ k[42], r22 ^ k[0], r23 ^ k[2],
		r24 ^ k[21], &l3, &l28, &l10, &l18);
	s7 (r23 ^ k[52], r24 ^ k[28], r25 ^ k[44], r26 ^ k[22], r27 ^ k[9],
		r28 ^ k[1], &l31, &l11, &l21, &l6);
	s8 (r27 ^ k[7], r28 ^ k[38], r29 ^ k[35], r30 ^ k[23], r31 ^ k[29],
		r0 ^ k[50], &l4, &l26, &l14, &l20);

	printf("\n Output 7: ");

    printf("%d",l31 & 0x01); printf("%d",l30 & 0x01); printf("%d",l29 & 0x01); printf("%d",l28 & 0x01);
    printf("%d",l27 & 0x01);printf("%d",l26 & 0x01);printf("%d",l25 & 0x01);printf("%d ",l24 & 0x01);
    printf("%d",l23 & 0x01);printf("%d",l22 & 0x01);printf("%d",l21 & 0x01);printf("%d",l20 & 0x01);
    printf("%d",l19 & 0x01);printf("%d",l18 & 0x01);printf("%d",l17 & 0x01);printf("%d ",l16 & 0x01);
    printf("%d",l15 & 0x01);printf("%d",l14 & 0x01);printf("%d",l13 & 0x01);printf("%d",l12 & 0x01);
    printf("%d",l11 & 0x01);printf("%d",l10 & 0x01);printf("%d",l9 & 0x01);printf("%d ",l8 & 0x01);
    printf("%d",l7 & 0x01);printf("%d",l6 & 0x01);printf("%d",l5 & 0x01);printf("%d",l4 & 0x01);
    printf("%d",l3 & 0x01);printf("%d",l2 & 0x01);printf("%d",l1 & 0x01);printf("%d ",l0 & 0x01);


	s1 (l31 ^ k[24], l0 ^ k[20], l1 ^ k[3], l2 ^ k[12], l3 ^ k[47],
		l4 ^ k[18], &r8, &r16, &r22, &r30);
	s2 (l3 ^ k[4], l4 ^ k[40], l5 ^ k[6], l6 ^ k[25], l7 ^ k[48],
		l8 ^ k[53], &r12, &r27, &r1, &r17);
	s3 (l7 ^ k[5], l8 ^ k[34], l9 ^ k[10], l10 ^ k[11], l11 ^ k[26],
		l12 ^ k[39], &r23, &r15, &r29, &r5);
	s4 (l11 ^ k[13], l12 ^ k[32], l13 ^ k[33], l14 ^ k[41], l15 ^ k[17],
		l16 ^ k[54], &r25, &r19, &r9, &r0);
	s5 (l15 ^ k[45], l16 ^ k[8], l17 ^ k[2], l18 ^ k[44], l19 ^ k[28],
		l20 ^ k[29], &r7, &r13, &r24, &r2);
	s6 (l19 ^ k[50], l20 ^ k[38], l21 ^ k[1], l22 ^ k[14], l23 ^ k[16],
		l24 ^ k[35], &r3, &r28, &r10, &r18);
	s7 (l23 ^ k[7], l24 ^ k[42], l25 ^ k[31], l26 ^ k[36], l27 ^ k[23],
		l28 ^ k[15], &r31, &r11, &r21, &r6);
	s8 (l27 ^ k[21], l28 ^ k[52], l29 ^ k[49], l30 ^ k[37], l31 ^ k[43], l0 ^ k[9],
		&r4, &r26, &r14, &r20);

    printf("\n Output 8: ");

    printf("%d",r31 & 0x01); printf("%d",r30 & 0x01); printf("%d",r29 & 0x01); printf("%d",r28 & 0x01);
    printf("%d",r27 & 0x01);printf("%d",r26 & 0x01);printf("%d",r25 & 0x01);printf("%d ",r24 & 0x01);
    printf("%d",r23 & 0x01);printf("%d",r22 & 0x01);printf("%d",r21 & 0x01);printf("%d",r20 & 0x01);
    printf("%d",r19 & 0x01);printf("%d",r18 & 0x01);printf("%d",r17 & 0x01);printf("%d ",r16 & 0x01);
    printf("%d",r15 & 0x01);printf("%d",r14 & 0x01);printf("%d",r13 & 0x01);printf("%d",r12 & 0x01);
    printf("%d",r11 & 0x01);printf("%d",r10 & 0x01);printf("%d",r9 & 0x01);printf("%d ",r8 & 0x01);
    printf("%d",r7 & 0x01);printf("%d",r6 & 0x01);printf("%d",r5 & 0x01);printf("%d",r4 & 0x01);
    printf("%d",r3 & 0x01);printf("%d",r2 & 0x01);printf("%d",r1 & 0x01);printf("%d ",r0 & 0x01);

    printf("\n");

	s1 (r31 ^ k[6], r0 ^ k[27], r1 ^ k[10], r2 ^ k[19], r3 ^ k[54],
		r4 ^ k[25], &l8, &l16, &l22, &l30);
	s2 (r3 ^ k[11], r4 ^ k[47], r5 ^ k[13], r6 ^ k[32], r7 ^ k[55],
		r8 ^ k[3], &l12, &l27, &l1, &l17);
	s3 (r7 ^ k[12], r8 ^ k[41], r9 ^ k[17], r10 ^ k[18], r11 ^ k[33],
		r12 ^ k[46], &l23, &l15, &l29, &l5);
	s4 (r11 ^ k[20], r12 ^ k[39], r13 ^ k[40], r14 ^ k[48], r15 ^ k[24],
		r16 ^ k[4], &l25, &l19, &l9, &l0);
	s5 (r15 ^ k[52], r16 ^ k[15], r17 ^ k[9], r18 ^ k[51], r19 ^ k[35],
		r20 ^ k[36], &l7, &l13, &l24, &l2);
	s6 (r19 ^ k[2], r20 ^ k[45], r21 ^ k[8], r22 ^ k[21], r23 ^ k[23],
		r24 ^ k[42], &l3, &l28, &l10, &l18);
	s7 (r23 ^ k[14], r24 ^ k[49], r25 ^ k[38], r26 ^ k[43], r27 ^ k[30],
		r28 ^ k[22], &l31, &l11, &l21, &l6);
	s8 (r27 ^ k[28], r28 ^ k[0], r29 ^ k[1], r30 ^ k[44], r31 ^ k[50],
		r0 ^ k[16], &l4, &l26, &l14, &l20);

    printf("\n Output 9: ");

    printf("%d",l31 & 0x01); printf("%d",l30 & 0x01); printf("%d",l29 & 0x01); printf("%d",l28 & 0x01);
    printf("%d",l27 & 0x01);printf("%d",l26 & 0x01);printf("%d",l25 & 0x01);printf("%d ",l24 & 0x01);
    printf("%d",l23 & 0x01);printf("%d",l22 & 0x01);printf("%d",l21 & 0x01);printf("%d",l20 & 0x01);
    printf("%d",l19 & 0x01);printf("%d",l18 & 0x01);printf("%d",l17 & 0x01);printf("%d ",l16 & 0x01);
    printf("%d",l15 & 0x01);printf("%d",l14 & 0x01);printf("%d",l13 & 0x01);printf("%d",l12 & 0x01);
    printf("%d",l11 & 0x01);printf("%d",l10 & 0x01);printf("%d",l9 & 0x01);printf("%d ",l8 & 0x01);
    printf("%d",l7 & 0x01);printf("%d",l6 & 0x01);printf("%d",l5 & 0x01);printf("%d",l4 & 0x01);
    printf("%d",l3 & 0x01);printf("%d",l2 & 0x01);printf("%d",l1 & 0x01);printf("%d ",l0 & 0x01);


	s1 (l31 ^ k[20], l0 ^ k[41], l1 ^ k[24], l2 ^ k[33], l3 ^ k[11],
		l4 ^ k[39], &r8, &r16, &r22, &r30);
	s2 (l3 ^ k[25], l4 ^ k[4], l5 ^ k[27], l6 ^ k[46], l7 ^ k[12],
		l8 ^ k[17], &r12, &r27, &r1, &r17);
	s3 (l7 ^ k[26], l8 ^ k[55], l9 ^ k[6], l10 ^ k[32], l11 ^ k[47],
		l12 ^ k[3], &r23, &r15, &r29, &r5);
	s4 (l11 ^ k[34], l12 ^ k[53], l13 ^ k[54], l14 ^ k[5], l15 ^ k[13],
		l16 ^ k[18], &r25, &r19, &r9, &r0);
	s5 (l15 ^ k[7], l16 ^ k[29], l17 ^ k[23], l18 ^ k[38], l19 ^ k[49],
		l20 ^ k[50], &r7, &r13, &r24, &r2);
	s6 (l19 ^ k[16], l20 ^ k[0], l21 ^ k[22], l22 ^ k[35], l23 ^ k[37],
		l24 ^ k[1], &r3, &r28, &r10, &r18);
	s7 (l23 ^ k[28], l24 ^ k[8], l25 ^ k[52], l26 ^ k[2], l27 ^ k[44],
		l28 ^ k[36], &r31, &r11, &r21, &r6);
	s8 (l27 ^ k[42], l28 ^ k[14], l29 ^ k[15], l30 ^ k[31], l31 ^ k[9],
		l0 ^ k[30], &r4, &r26, &r14, &r20);


    printf("\n Output 10: ");

    printf("%d",r31 & 0x01); printf("%d",r30 & 0x01); printf("%d",r29 & 0x01); printf("%d",r28 & 0x01);
    printf("%d",r27 & 0x01);printf("%d",r26 & 0x01);printf("%d",r25 & 0x01);printf("%d ",r24 & 0x01);
    printf("%d",r23 & 0x01);printf("%d",r22 & 0x01);printf("%d",r21 & 0x01);printf("%d",r20 & 0x01);
    printf("%d",r19 & 0x01);printf("%d",r18 & 0x01);printf("%d",r17 & 0x01);printf("%d ",r16 & 0x01);
    printf("%d",r15 & 0x01);printf("%d",r14 & 0x01);printf("%d",r13 & 0x01);printf("%d",r12 & 0x01);
    printf("%d",r11 & 0x01);printf("%d",r10 & 0x01);printf("%d",r9 & 0x01);printf("%d ",r8 & 0x01);
    printf("%d",r7 & 0x01);printf("%d",r6 & 0x01);printf("%d",r5 & 0x01);printf("%d",r4 & 0x01);
    printf("%d",r3 & 0x01);printf("%d",r2 & 0x01);printf("%d",r1 & 0x01);printf("%d ",r0 & 0x01);

    printf("\n");

	s1 (r31 ^ k[34], r0 ^ k[55], r1 ^ k[13], r2 ^ k[47], r3 ^ k[25],
		r4 ^ k[53], &l8, &l16, &l22, &l30);
	s2 (r3 ^ k[39], r4 ^ k[18], r5 ^ k[41], r6 ^ k[3], r7 ^ k[26],
		r8 ^ k[6], &l12, &l27, &l1, &l17);
	s3 (r7 ^ k[40], r8 ^ k[12], r9 ^ k[20], r10 ^ k[46], r11 ^ k[4],
		r12 ^ k[17], &l23, &l15, &l29, &l5);
	s4 (r11 ^ k[48], r12 ^ k[10], r13 ^ k[11], r14 ^ k[19], r15 ^ k[27],
		r16 ^ k[32], &l25, &l19, &l9, &l0);
	s5 (r15 ^ k[21], r16 ^ k[43], r17 ^ k[37], r18 ^ k[52], r19 ^ k[8],
		r20 ^ k[9], &l7, &l13, &l24, &l2);
	s6 (r19 ^ k[30], r20 ^ k[14], r21 ^ k[36], r22 ^ k[49], r23 ^ k[51],
		r24 ^ k[15], &l3, &l28, &l10, &l18);
	s7 (r23 ^ k[42], r24 ^ k[22], r25 ^ k[7], r26 ^ k[16], r27 ^ k[31],
		r28 ^ k[50], &l31, &l11, &l21, &l6);
	s8 (r27 ^ k[1], r28 ^ k[28], r29 ^ k[29], r30 ^ k[45], r31 ^ k[23],
		r0 ^ k[44], &l4, &l26, &l14, &l20);

    printf("\n Output 11: ");

    printf("%d",l31 & 0x01); printf("%d",l30 & 0x01); printf("%d",l29 & 0x01); printf("%d",l28 & 0x01);
    printf("%d",l27 & 0x01);printf("%d",l26 & 0x01);printf("%d",l25 & 0x01);printf("%d ",l24 & 0x01);
    printf("%d",l23 & 0x01);printf("%d",l22 & 0x01);printf("%d",l21 & 0x01);printf("%d",l20 & 0x01);
    printf("%d",l19 & 0x01);printf("%d",l18 & 0x01);printf("%d",l17 & 0x01);printf("%d ",l16 & 0x01);
    printf("%d",l15 & 0x01);printf("%d",l14 & 0x01);printf("%d",l13 & 0x01);printf("%d",l12 & 0x01);
    printf("%d",l11 & 0x01);printf("%d",l10 & 0x01);printf("%d",l9 & 0x01);printf("%d ",l8 & 0x01);
    printf("%d",l7 & 0x01);printf("%d",l6 & 0x01);printf("%d",l5 & 0x01);printf("%d",l4 & 0x01);
    printf("%d",l3 & 0x01);printf("%d",l2 & 0x01);printf("%d",l1 & 0x01);printf("%d ",l0 & 0x01);



	s1 (l31 ^ k[48], l0 ^ k[12], l1 ^ k[27], l2 ^ k[4], l3 ^ k[39],
		l4 ^ k[10], &r8, &r16, &r22, &r30);
	s2 (l3 ^ k[53], l4 ^ k[32], l5 ^ k[55], l6 ^ k[17], l7 ^ k[40],
		l8 ^ k[20], &r12, &r27, &r1, &r17);
	s3 (l7 ^ k[54], l8 ^ k[26], l9 ^ k[34], l10 ^ k[3], l11 ^ k[18],
		l12 ^ k[6], &r23, &r15, &r29, &r5);
	s4 (l11 ^ k[5], l12 ^ k[24], l13 ^ k[25], l14 ^ k[33], l15 ^ k[41],
		l16 ^ k[46], &r25, &r19, &r9, &r0);
	s5 (l15 ^ k[35], l16 ^ k[2], l17 ^ k[51], l18 ^ k[7], l19 ^ k[22],
		l20 ^ k[23], &r7, &r13, &r24, &r2);
	s6 (l19 ^ k[44], l20 ^ k[28], l21 ^ k[50], l22 ^ k[8], l23 ^ k[38],
		l24 ^ k[29], &r3, &r28, &r10, &r18);
	s7 (l23 ^ k[1], l24 ^ k[36], l25 ^ k[21], l26 ^ k[30], l27 ^ k[45],
		l28 ^ k[9], &r31, &r11, &r21, &r6);
	s8 (l27 ^ k[15], l28 ^ k[42], l29 ^ k[43], l30 ^ k[0], l31 ^ k[37],
		l0 ^ k[31], &r4, &r26, &r14, &r20);

    printf("\n Output 12: ");

    printf("%d",r31 & 0x01); printf("%d",r30 & 0x01); printf("%d",r29 & 0x01); printf("%d",r28 & 0x01);
    printf("%d",r27 & 0x01);printf("%d",r26 & 0x01);printf("%d",r25 & 0x01);printf("%d ",r24 & 0x01);
    printf("%d",r23 & 0x01);printf("%d",r22 & 0x01);printf("%d",r21 & 0x01);printf("%d",r20 & 0x01);
    printf("%d",r19 & 0x01);printf("%d",r18 & 0x01);printf("%d",r17 & 0x01);printf("%d ",r16 & 0x01);
    printf("%d",r15 & 0x01);printf("%d",r14 & 0x01);printf("%d",r13 & 0x01);printf("%d",r12 & 0x01);
    printf("%d",r11 & 0x01);printf("%d",r10 & 0x01);printf("%d",r9 & 0x01);printf("%d ",r8 & 0x01);
    printf("%d",r7 & 0x01);printf("%d",r6 & 0x01);printf("%d",r5 & 0x01);printf("%d",r4 & 0x01);
    printf("%d",r3 & 0x01);printf("%d",r2 & 0x01);printf("%d",r1 & 0x01);printf("%d ",r0 & 0x01);

    printf("\n");


	s1 (r31 ^ k[5], r0 ^ k[26], r1 ^ k[41], r2 ^ k[18], r3 ^ k[53],
		r4 ^ k[24], &l8, &l16, &l22, &l30);
	s2 (r3 ^ k[10], r4 ^ k[46], r5 ^ k[12], r6 ^ k[6], r7 ^ k[54],
		r8 ^ k[34], &l12, &l27, &l1, &l17);
	s3 (r7 ^ k[11], r8 ^ k[40], r9 ^ k[48], r10 ^ k[17], r11 ^ k[32],
		r12 ^ k[20], &l23, &l15, &l29, &l5);
	s4 (r11 ^ k[19], r12 ^ k[13], r13 ^ k[39], r14 ^ k[47], r15 ^ k[55],
		r16 ^ k[3], &l25, &l19, &l9, &l0);
	s5 (r15 ^ k[49], r16 ^ k[16], r17 ^ k[38], r18 ^ k[21], r19 ^ k[36],
		r20 ^ k[37], &l7, &l13, &l24, &l2);
	s6 (r19 ^ k[31], r20 ^ k[42], r21 ^ k[9], r22 ^ k[22], r23 ^ k[52],
		r24 ^ k[43], &l3, &l28, &l10, &l18);
	s7 (r23 ^ k[15], r24 ^ k[50], r25 ^ k[35], r26 ^ k[44], r27 ^ k[0],
		r28 ^ k[23], &l31, &l11, &l21, &l6);
	s8 (r27 ^ k[29], r28 ^ k[1], r29 ^ k[2], r30 ^ k[14], r31 ^ k[51],
		r0 ^ k[45], &l4, &l26, &l14, &l20);


    printf("\n Output 13: ");

    printf("%d",l31 & 0x01); printf("%d",l30 & 0x01); printf("%d",l29 & 0x01); printf("%d",l28 & 0x01);
    printf("%d",l27 & 0x01);printf("%d",l26 & 0x01);printf("%d",l25 & 0x01);printf("%d ",l24 & 0x01);
    printf("%d",l23 & 0x01);printf("%d",l22 & 0x01);printf("%d",l21 & 0x01);printf("%d",l20 & 0x01);
    printf("%d",l19 & 0x01);printf("%d",l18 & 0x01);printf("%d",l17 & 0x01);printf("%d ",l16 & 0x01);
    printf("%d",l15 & 0x01);printf("%d",l14 & 0x01);printf("%d",l13 & 0x01);printf("%d",l12 & 0x01);
    printf("%d",l11 & 0x01);printf("%d",l10 & 0x01);printf("%d",l9 & 0x01);printf("%d ",l8 & 0x01);
    printf("%d",l7 & 0x01);printf("%d",l6 & 0x01);printf("%d",l5 & 0x01);printf("%d",l4 & 0x01);
    printf("%d",l3 & 0x01);printf("%d",l2 & 0x01);printf("%d",l1 & 0x01);printf("%d ",l0 & 0x01);



	s1 (l31 ^ k[19], l0 ^ k[40], l1 ^ k[55], l2 ^ k[32], l3 ^ k[10],
		l4 ^ k[13], &r8, &r16, &r22, &r30);
	s2 (l3 ^ k[24], l4 ^ k[3], l5 ^ k[26], l6 ^ k[20], l7 ^ k[11],
		l8 ^ k[48], &r12, &r27, &r1, &r17);
	s3 (l7 ^ k[25], l8 ^ k[54], l9 ^ k[5], l10 ^ k[6], l11 ^ k[46],
		l12 ^ k[34], &r23, &r15, &r29, &r5);
	s4 (l11 ^ k[33], l12 ^ k[27], l13 ^ k[53], l14 ^ k[4], l15 ^ k[12],
		l16 ^ k[17], &r25, &r19, &r9, &r0);
	s5 (l15 ^ k[8], l16 ^ k[30], l17 ^ k[52], l18 ^ k[35], l19 ^ k[50],
		l20 ^ k[51], &r7, &r13, &r24, &r2);
	s6 (l19 ^ k[45], l20 ^ k[1], l21 ^ k[23], l22 ^ k[36], l23 ^ k[7],
		l24 ^ k[2], &r3, &r28, &r10, &r18);
	s7 (l23 ^ k[29], l24 ^ k[9], l25 ^ k[49], l26 ^ k[31], l27 ^ k[14],
		l28 ^ k[37], &r31, &r11, &r21, &r6);
	s8 (l27 ^ k[43], l28 ^ k[15], l29 ^ k[16], l30 ^ k[28], l31 ^ k[38],
		l0 ^ k[0], &r4, &r26, &r14, &r20);

    printf("\n Output 14: ");

    printf("%d",r31 & 0x01); printf("%d",r30 & 0x01); printf("%d",r29 & 0x01); printf("%d",r28 & 0x01);
    printf("%d",r27 & 0x01);printf("%d",r26 & 0x01);printf("%d",r25 & 0x01);printf("%d ",r24 & 0x01);
    printf("%d",r23 & 0x01);printf("%d",r22 & 0x01);printf("%d",r21 & 0x01);printf("%d",r20 & 0x01);
    printf("%d",r19 & 0x01);printf("%d",r18 & 0x01);printf("%d",r17 & 0x01);printf("%d ",r16 & 0x01);
    printf("%d",r15 & 0x01);printf("%d",r14 & 0x01);printf("%d",r13 & 0x01);printf("%d",r12 & 0x01);
    printf("%d",r11 & 0x01);printf("%d",r10 & 0x01);printf("%d",r9 & 0x01);printf("%d ",r8 & 0x01);
    printf("%d",r7 & 0x01);printf("%d",r6 & 0x01);printf("%d",r5 & 0x01);printf("%d",r4 & 0x01);
    printf("%d",r3 & 0x01);printf("%d",r2 & 0x01);printf("%d",r1 & 0x01);printf("%d ",r0 & 0x01);

    printf("\n");


	s1 (r31 ^ k[33], r0 ^ k[54], r1 ^ k[12], r2 ^ k[46], r3 ^ k[24],
		r4 ^ k[27], &l8, &l16, &l22, &l30);

	s2 (r3 ^ k[13], r4 ^ k[17], r5 ^ k[40], r6 ^ k[34], r7 ^ k[25],
		r8 ^ k[5], &l12, &l27, &l1, &l17);

	s3 (r7 ^ k[39], r8 ^ k[11], r9 ^ k[19], r10 ^ k[20], r11 ^ k[3],
		r12 ^ k[48], &l23, &l15, &l29, &l5);

	s4 (r11 ^ k[47], r12 ^ k[41], r13 ^ k[10], r14 ^ k[18], r15 ^ k[26],
		r16 ^ k[6], &l25, &l19, &l9, &l0);

	s5 (r15 ^ k[22], r16 ^ k[44], r17 ^ k[7], r18 ^ k[49], r19 ^ k[9],
		r20 ^ k[38], &l7, &l13, &l24, &l2);

	s6 (r19 ^ k[0], r20 ^ k[15], r21 ^ k[37], r22 ^ k[50], r23 ^ k[21],
		r24 ^ k[16], &l3, &l28, &l10, &l18);

	s7 (r23 ^ k[43], r24 ^ k[23], r25 ^ k[8], r26 ^ k[45], r27 ^ k[28],
		r28 ^ k[51], &l31, &l11, &l21, &l6);

	s8 (r27 ^ k[2], r28 ^ k[29], r29 ^ k[30], r30 ^ k[42], r31 ^ k[52],
		r0 ^ k[14], &l4, &l26, &l14, &l20);

    printf("\n Output 15: ");

    printf("%d",l31 & 0x01); printf("%d",l30 & 0x01); printf("%d",l29 & 0x01); printf("%d",l28 & 0x01);
    printf("%d",l27 & 0x01);printf("%d",l26 & 0x01);printf("%d",l25 & 0x01);printf("%d ",l24 & 0x01);
    printf("%d",l23 & 0x01);printf("%d",l22 & 0x01);printf("%d",l21 & 0x01);printf("%d",l20 & 0x01);
    printf("%d",l19 & 0x01);printf("%d",l18 & 0x01);printf("%d",l17 & 0x01);printf("%d ",l16 & 0x01);
    printf("%d",l15 & 0x01);printf("%d",l14 & 0x01);printf("%d",l13 & 0x01);printf("%d",l12 & 0x01);
    printf("%d",l11 & 0x01);printf("%d",l10 & 0x01);printf("%d",l9 & 0x01);printf("%d ",l8 & 0x01);
    printf("%d",l7 & 0x01);printf("%d",l6 & 0x01);printf("%d",l5 & 0x01);printf("%d",l4 & 0x01);
    printf("%d",l3 & 0x01);printf("%d",l2 & 0x01);printf("%d",l1 & 0x01);printf("%d ",l0 & 0x01);



	s1 (l31 ^ k[40], l0 ^ k[4], l1 ^ k[19], l2 ^ k[53], l3 ^ k[6],
		l4 ^ k[34], &r8, &r16, &r22, &r30);

	s2 (l3 ^ k[20], l4 ^ k[24], l5 ^ k[47], l6 ^ k[41], l7 ^ k[32],
		l8 ^ k[12], &r12, &r27, &r1, &r17);

	s3 (l7 ^ k[46], l8 ^ k[18], l9 ^ k[26], l10 ^ k[27], l11 ^ k[10],
		l12 ^ k[55], &r23, &r15, &r29, &r5);

	s4 (l11 ^ k[54], l12 ^ k[48], l13 ^ k[17], l14 ^ k[25], l15 ^ k[33],
		l16 ^ k[13], &r25, &r19, &r9, &r0);

	s5 (l15 ^ k[29], l16 ^ k[51], l17 ^ k[14], l18 ^ k[1], l19 ^ k[16],
		l20 ^ k[45], &r7, &r13, &r24, &r2);

	s6 (l19 ^ k[7], l20 ^ k[22], l21 ^ k[44], l22 ^ k[2], l23 ^ k[28],
		l24 ^ k[23], &r3, &r28, &r10, &r18);

	s7 (l23 ^ k[50], l24 ^ k[30], l25 ^ k[15], l26 ^ k[52], l27 ^ k[35],
		l28 ^ k[31], &r31, &r11, &r21, &r6);

	s8 (l27 ^ k[9], l28 ^ k[36], l29 ^ k[37], l30 ^ k[49], l31 ^ k[0],
		l0 ^ k[21], &r4, &r26, &r14, &r20);

    printf("\n Output 16: ");

    printf("%d",r31 & 0x01); printf("%d",r30 & 0x01); printf("%d",r29 & 0x01); printf("%d",r28 & 0x01);
    printf("%d",r27 & 0x01);printf("%d",r26 & 0x01);printf("%d",r25 & 0x01);printf("%d ",r24 & 0x01);
    printf("%d",r23 & 0x01);printf("%d",r22 & 0x01);printf("%d",r21 & 0x01);printf("%d",r20 & 0x01);
    printf("%d",r19 & 0x01);printf("%d",r18 & 0x01);printf("%d",r17 & 0x01);printf("%d ",r16 & 0x01);
    printf("%d",r15 & 0x01);printf("%d",r14 & 0x01);printf("%d",r13 & 0x01);printf("%d",r12 & 0x01);
    printf("%d",r11 & 0x01);printf("%d",r10 & 0x01);printf("%d",r9 & 0x01);printf("%d ",r8 & 0x01);
    printf("%d",r7 & 0x01);printf("%d",r6 & 0x01);printf("%d",r5 & 0x01);printf("%d",r4 & 0x01);
    printf("%d",r3 & 0x01);printf("%d",r2 & 0x01);printf("%d",r1 & 0x01);printf("%d ",r0 & 0x01);

    printf("\n");

}

char desverify(UINTX *c)
{
    char	result = 0;
    //char string[100];
    //ltoa(c[59],string,2);
    //printf("%s\n",string);
    //ltoa(l23,string,2);
    //printf("%s\n",string);

    if (l8 ^ c[5]) result++;
    if (l16 ^ c[3]) result++;
    if (l22 ^ c[51]) result++;
    if (l30 ^ c[49]) result++;


    if (l12 ^ c[37]) result++;
    if (l27 ^ c[25]) result++;
    if (l1 ^ c[15]) result++;
    if (l17 ^ c[11]) result++;

    if (l23 ^ c[59]) result++;
    if (l15 ^ c[61]) result++;
    if (l29 ^ c[41]) result++;
    if (l5 ^ c[47]) result++;

    if (l25 ^ c[9]) result++;
    if (l19 ^ c[27]) result++;
    if (l9 ^ c[13]) result++;
    if (l0 ^ c[7]) result++;

    if (l7 ^ c[63]) result++;
    if (l13 ^ c[45]) result++;
    if (l24 ^ c[1]) result++;
    if (l2 ^ c[23]) result++;

    if (l3 ^ c[31]) result++;
    if (l28 ^ c[33]) result++;
    if (l10 ^ c[21]) result++;
    if (l18 ^ c[19]) result++;

    if (l31 ^ c[57]) result++;
    if (l11 ^ c[29]) result++;
    if (l21 ^ c[43]) result++;
    if (l6 ^ c[55]) result++;

    if (l4 ^ c[39]) result++;
    if (l26 ^ c[17]) result++;
    if (l14 ^ c[53]) result++;
    if (l20 ^ c[35]) result++;

    if (r8 ^ c[4]) result++;
    if (r16 ^ c[2]) result++;
    if (r22 ^ c[50]) result++;
    if (r30 ^ c[48]) result++;

    if (r12 ^ c[36]) result++;
    if (r27 ^ c[24]) result++;
    if (r1 ^ c[14]) result++;
    if (r17 ^ c[10]) result++;

    if (r23 ^ c[58]) result++;
    if (r15 ^ c[60]) result++;
    if (r29 ^ c[40]) result++;
    if (r5 ^ c[46]) result++;

    if (r25 ^ c[8]) result++;
    if (r19 ^ c[26]) result++;
    if (r9 ^ c[12]) result++;
    if (r0 ^ c[6]) result++;

    if (r7 ^ c[62]) result++;
    if (r13 ^ c[44]) result++;
    if (r24 ^ c[0]) result++;
    if (r2 ^ c[22]) result++;

    if (r3 ^ c[30]) result++;
    if (r28 ^ c[32]) result++;
    if (r10 ^ c[20]) result++;
    if (r18 ^ c[18]) result++;

    if (r31 ^ c[56]) result++;
    if (r11 ^ c[28]) result++;
    if (r21 ^ c[42]) result++;
    if (r6 ^ c[54]) result++;

    if (r4 ^ c[38]) result++;
    if (r26 ^ c[16]) result++;
    if (r14 ^ c[52]) result++;
    if (r20 ^ c[34]) result++;

	return (result);
}
