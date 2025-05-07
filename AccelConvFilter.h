
#ifndef ACCEL_CONV_FILTER_H
#define ACCEL_CONV_FILTER_H

#include <hls_stream.h>
#include <iostream>

#include "ap_int.h"
#include "ap_axi_sdata.h"
#include "ap_fixed.h"

#include <hls_math.h>

typedef ap_uint<32>    UINT32;
typedef ap_uint<24>    UINT24;
typedef ap_uint<8>     Bits8;
typedef ap_int<8>      INT8;
typedef ap_uint<1>     Bit;
typedef ap_fixed<16,8> FLOAT;

// *******************************

#define MAX_MAT_SIZE  8
#define MAX_FILT_SIZE_3  3
#define MAX_FILT_SIZE_5  5
#define MAX_FILT_SIZE_9  9

#define MATRIX_SIZE 8
#define FILTER_SIZE_3 3
#define FILTER_SIZE_5 5

//1280x720
const unsigned int MAX_NUM_LINES = 480;         // Num of Lines one HD Frame
const unsigned int MAX_NUM_PIX_PER_LINE = 640;  // Num of pixels per line in HD resolution
const unsigned int BUFFER_ARRAY_SIZE = 16;      // Chunk of lines of frames to process due to limited BRAM memory

const unsigned int MAX_FILTER_SIZE = 81; //9x9

// TRIPCOUNT identifier

typedef ap_axiu< 24,1,0,0, AXIS_ENABLE_KEEP | AXIS_ENABLE_LAST> rgbPix;
typedef ap_axiu<  8,1,0,0, AXIS_ENABLE_KEEP | AXIS_ENABLE_LAST> grayPix;

// top function
void AccelConvFilter(hls::stream<grayPix>& S_AXIS_IN, hls::stream<rgbPix>& M_AXIS_OUT,
		 	 	 	 unsigned int* MatRowsP, unsigned int* MatColsP,
					 unsigned int* PassThroughP,
					 int* FilterIn, unsigned int* FilterInDimX, unsigned int* FilterInDimY);
//					 int* SetFilter,  int*  FiltMemInAddr, int* FiltSizeP);

void StoreLineSendPix(hls::stream<grayPix>& S_AXIS_IN,
					  unsigned char* LocalMatIn, ap_uint<4> BuffTailIndex,
					  hls::stream<rgbPix>& M_AXIS_OUT);

void ProcLine(hls::stream<grayPix>& S_AXIS_IN, unsigned char* LocalMatIn,
			  unsigned int RealLineIndex, ap_uint<4> BuffHeadIndex, ap_uint<4> BuffTailIndex,
			  hls::stream<rgbPix>& M_AXIS_OUT,
			  int* Filter, unsigned int FilterDim,
			  bool& IsLastPix);

unsigned int ApplyFilters(  unsigned char MatIn[], int i, int j, int PixPerLine,
		int* Filter, unsigned int FilterDim);

void ApplyFilterDim3X( unsigned char MatIn[], int i, int j, int PixPerLine,
		int* FilterX, unsigned int FilterDim,
					   ap_int<32>* Results);

void ApplyFilterDim3Y( unsigned char MatIn[], int i, int j, int PixPerLine,
					   int* FilterY, unsigned int FilterDim,
					   ap_int<32>* Results);

void PassThrough(hls::stream<grayPix>& S_AXIS_IN, hls::stream<rgbPix>& M_AXIS_OUT);

#if 0
void ProcessLine(unsigned char* LocalMatIn, unsigned short LineIndex,
				 grayPix& tmp, bool IsLastLine, hls::stream<rgbPix>& M_AXIS_OUT);

void DummyLine(grayPix& tmp, bool IsLastLine, hls::stream<rgbPix>& M_AXIS_OUT);

#endif

#if 0

void TraverseMatrix3(int* LocalMatIn, int* LocalMatOut);
int  ApplyFilterDim3(int* LocalMatIn, int i, int j);

void StreamDataOut(unsigned char* LocalMatOut, int NumPixels, hls::stream<grayPix>& M_AXIS_OUT, bool HasLastPixel);
#endif

#endif
