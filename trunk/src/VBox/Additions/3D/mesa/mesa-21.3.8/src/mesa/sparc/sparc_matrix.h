/*
 * SPARC assembly matrix code.
 */

#ifndef _SPARC_MATRIX_H
#define _SPARC_MATRIX_H

#ifdef __arch64__
#define LDPTR		ldx
#define MATH_ASM_PTR_SIZE 8
#include "math/m_vector_asm.h"
#else
#define LDPTR		ld
#define MATH_ASM_PTR_SIZE 4
#include "math/m_vector_asm.h"
#endif

#define M0		%f16
#define M1		%f17
#define M2		%f18
#define M3		%f19
#define M4		%f20
#define M5		%f21
#define M6		%f22
#define M7		%f23
#define M8		%f24
#define M9		%f25
#define M10		%f26
#define M11		%f27
#define M12		%f28
#define M13		%f29
#define M14		%f30
#define M15		%f31

#define LDMATRIX_0_1_2_3_12_13_14_15(BASE)	\
	ldd	[BASE + ( 0 * 0x4)], M0;	\
	ldd	[BASE + ( 2 * 0x4)], M2;	\
	ldd	[BASE + (12 * 0x4)], M12;	\
	ldd	[BASE + (14 * 0x4)], M14

#define LDMATRIX_0_1_12_13(BASE)		\
	ldd	[BASE + ( 0 * 0x4)], M0;	\
	ldd	[BASE + (12 * 0x4)], M12

#define LDMATRIX_0_12_13(BASE)			\
	ld	[BASE + ( 0 * 0x4)], M0;	\
	ldd	[BASE + (12 * 0x4)], M12

#define LDMATRIX_0_1_2_12_13_14(BASE)		\
	ldd	[BASE + ( 0 * 0x4)], M0;	\
	ld	[BASE + ( 2 * 0x4)], M2;	\
	ldd	[BASE + (12 * 0x4)], M12;	\
	ld	[BASE + (14 * 0x4)], M14

#define LDMATRIX_0_12_13_14(BASE)		\
	ld	[BASE + ( 0 * 0x4)], M0;	\
	ldd	[BASE + (12 * 0x4)], M12;	\
	ld	[BASE + (14 * 0x4)], M14

#define LDMATRIX_0_14(BASE)			\
	ld	[BASE + ( 0 * 0x4)], M0;	\
	ld	[BASE + (14 * 0x4)], M14

#define LDMATRIX_0_1_2_3_4_5_6_7_12_13_14_15(BASE) \
	ldd	[BASE + ( 0 * 0x4)], M0;	\
	ldd	[BASE + ( 2 * 0x4)], M2;	\
	ldd	[BASE + ( 4 * 0x4)], M4;	\
	ldd	[BASE + ( 6 * 0x4)], M6;	\
	ldd	[BASE + (12 * 0x4)], M12;	\
	ldd	[BASE + (14 * 0x4)], M14

#define LDMATRIX_0_5_12_13(BASE) 		\
	ld	[BASE + ( 0 * 0x4)], M0;	\
	ld	[BASE + ( 5 * 0x4)], M5;	\
	ldd	[BASE + (12 * 0x4)], M12

#define LDMATRIX_0_1_2_3_4_5_6_12_13_14(BASE)	\
	ldd	[BASE + ( 0 * 0x4)], M0;	\
	ldd	[BASE + ( 2 * 0x4)], M2;	\
	ldd	[BASE + ( 4 * 0x4)], M4;	\
	ld	[BASE + ( 6 * 0x4)], M6;	\
	ldd	[BASE + (12 * 0x4)], M12;	\
	ld	[BASE + (14 * 0x4)], M14

#define LDMATRIX_0_5_12_13_14(BASE)		\
	ld	[BASE + ( 0 * 0x4)], M0;	\
	ld	[BASE + ( 5 * 0x4)], M5;	\
	ldd	[BASE + (12 * 0x4)], M12;	\
	ld	[BASE + (14 * 0x4)], M14

#define LDMATRIX_0_5_14(BASE)			\
	ld	[BASE + ( 0 * 0x4)], M0;	\
	ld	[BASE + ( 5 * 0x4)], M5;	\
	ld	[BASE + (14 * 0x4)], M14

#define LDMATRIX_0_1_2_3_4_5_6_7_8_9_10_11_12_13_14_15(BASE) \
	ldd	[BASE + ( 0 * 0x4)], M0;	\
	ldd	[BASE + ( 2 * 0x4)], M2;	\
	ldd	[BASE + ( 4 * 0x4)], M4;	\
	ldd	[BASE + ( 6 * 0x4)], M6;	\
	ldd	[BASE + ( 8 * 0x4)], M8;	\
	ldd	[BASE + (10 * 0x4)], M10;	\
	ldd	[BASE + (12 * 0x4)], M12;	\
	ldd	[BASE + (14 * 0x4)], M14

#define LDMATRIX_0_1_4_5_12_13(BASE) 		\
	ldd	[BASE + ( 0 * 0x4)], M0;	\
	ldd	[BASE + ( 4 * 0x4)], M4;	\
	ldd	[BASE + (12 * 0x4)], M12

#define LDMATRIX_0_5_12_13(BASE) 		\
	ld	[BASE + ( 0 * 0x4)], M0;	\
	ld	[BASE + ( 5 * 0x4)], M5;	\
	ldd	[BASE + (12 * 0x4)], M12

#define LDMATRIX_0_1_2_4_5_6_8_9_10(BASE) \
	ldd	[BASE + ( 0 * 0x4)], M0;	\
	ld	[BASE + ( 2 * 0x4)], M2;	\
	ldd	[BASE + ( 4 * 0x4)], M4;	\
	ld	[BASE + ( 6 * 0x4)], M6;	\
	ldd	[BASE + ( 8 * 0x4)], M8;	\
	ld	[BASE + (10 * 0x4)], M10

#define LDMATRIX_0_1_2_4_5_6_8_9_10_12_13_14(BASE) \
	ldd	[BASE + ( 0 * 0x4)], M0;	\
	ld	[BASE + ( 2 * 0x4)], M2;	\
	ldd	[BASE + ( 4 * 0x4)], M4;	\
	ld	[BASE + ( 6 * 0x4)], M6;	\
	ldd	[BASE + ( 8 * 0x4)], M8;	\
	ld	[BASE + (10 * 0x4)], M10;	\
	ldd	[BASE + (12 * 0x4)], M12;	\
	ld	[BASE + (14 * 0x4)], M14

#define LDMATRIX_0_5_10(BASE) 			\
	ld	[BASE + ( 0 * 0x4)], M0;	\
	ld	[BASE + ( 5 * 0x4)], M5;	\
	ld	[BASE + (10 * 0x4)], M10;

#define LDMATRIX_0_5_10_12_13_14(BASE) 		\
	ld	[BASE + ( 0 * 0x4)], M0;	\
	ld	[BASE + ( 5 * 0x4)], M5;	\
	ld	[BASE + (10 * 0x4)], M10;	\
	ldd	[BASE + (12 * 0x4)], M12;	\
	ld	[BASE + (14 * 0x4)], M14

#define LDMATRIX_0_5_8_9_10_14(BASE) 		\
	ld	[BASE + ( 0 * 0x4)], M0;	\
	ld	[BASE + ( 5 * 0x4)], M5;	\
	ldd	[BASE + ( 8 * 0x4)], M8;	\
	ld	[BASE + (10 * 0x4)], M10;	\
	ld	[BASE + (14 * 0x4)], M14

#endif /* !(_SPARC_MATRIX_H) */
