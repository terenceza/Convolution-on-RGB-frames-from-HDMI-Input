
#ifndef ACCEL_CONVOLUTE_H
#define ACCEL_CONVOLUTE_H

#include <vector>
#include <hls_vector.h>
#include <hls_stream.h>
#include "ap_axi_sdata.h"
#include <iostream>

#include "ap_int.h"
#include "ap_axi_sdata.h"

// *******************************

#define MAX_MAT_SIZE  8
#define MAX_FILT_SIZE_3  3
#define MAX_FILT_SIZE_5  5

#define MATRIX_SIZE 8
#define FILTER_SIZE_3 3
#define FILTER_SIZE_5 5

// TRIPCOUNT identifier

//typedef ap_axiu<24,0,0,0> pkt;

typedef hls::axis_data<int, AXIS_ENABLE_LAST> pkt;

// top function
void AccelConvolute(hls::stream<pkt>& S_AXIS_IN, hls::stream<pkt>& M_AXIS_OUT); //, hls::stream<pkt>& M_AXIS_OUT_1);
void PreProcess(hls::stream<pkt>& S_AXIS_IN, hls::stream<pkt>& S_OUT);

void LoadData(hls::stream<pkt>& S_AXIS_IN, int* LocalMatIn); //, int* LocalMatIn1);

void StreamDataOut(int* LocalMatOut, hls::stream<pkt>& M_AXIS_OUT);
void StreamDataOut_1(int* LocalMatOut, hls::stream<pkt>& M_AXIS_OUT);

void TraverseMatrix3(int* LocalMatIn, int* LocalMatOut);
void TraverseMatrix5(int* LocalMatIn, int* LocalMatOut);

int ApplyFilterDim3(int* LocalMatIn, int i, int j);
int ApplyFilterDim5(int* LocalMatIn, int i, int j);

#endif
