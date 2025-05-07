
#ifndef ACCEL_CONVOLUTE_H
#define ACCEL_CONVOLUTE_H

#include <iostream>
#include <hls_stream.h>
#include "ap_int.h"
#include "ap_axi_sdata.h"

#include <hls_math.h>


// TRIPCOUNT identifier

typedef ap_uint<32>    UINT32;
typedef ap_uint<24>    UINT24;
typedef ap_uint<8>     UINT8;
typedef ap_int<8>      INT8;
typedef ap_uint<1>     Bit;
typedef ap_fixed<16,8> FLOAT;

typedef ap_axiu<24,1,0,0, AXIS_ENABLE_KEEP | AXIS_ENABLE_LAST> rgbPixel;
typedef ap_axiu< 8,1,0,0, AXIS_ENABLE_KEEP | AXIS_ENABLE_LAST> grayPixel;

// top function
void AccelConvolute1(hls::stream<rgbPixel>& S_AXIS_IN, hls::stream<grayPixel>& M_AXIS_OUT,
		 	 	 	 unsigned int* FilterID);


#endif
