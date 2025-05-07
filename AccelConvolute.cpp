

//#include "ap_axi_sdata.h" // ap_axis can also be used, but it will include all sideband signals which we don't need
#include "hls_stream.h"
#include "ap_int.h"
#include "ap_axi_sdata.h"
#include "AccelConvolute.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

const unsigned int MaxMatSize = MATRIX_SIZE;
const unsigned int ChunkRowsSize = 3;
int count = 0;



void AccelConvolute(hls::stream<pkt>& S_AXIS_IN, hls::stream<pkt>& M_AXIS_OUT) //, hls::stream<pkt>& M_AXIS_OUT_1)
{

    #pragma HLS INTERFACE ap_ctrl_none port=return
	//#pragma HLS INTERFACE s_axilite port=return bundle=CONTROL_BUS
	#pragma HLS INTERFACE mode=axis port=S_AXIS_IN  name=IN_STREAM
	#pragma HLS INTERFACE mode=axis port=M_AXIS_OUT name=OUT_STREAM
	//#pragma HLS INTERFACE mode=axis port=M_AXIS_OUT_1 name=OUT_STREAM_1

	pkt Input;
	pkt Output;

	int LocalMatIn[MaxMatSize * MaxMatSize];
	//int LocalMatIn1[MaxMatSize * MaxMatSize];

	int LocalMatOut[MaxMatSize * MaxMatSize];
	//int LocalMatOut1[MaxMatSize * MaxMatSize];

	int k = 0; //index in the array

	hls::stream<pkt> S_OUT;


	//PreProcess(S_AXIS_IN, S_OUT);
	LoadData(S_AXIS_IN, LocalMatIn); //, LocalMatIn1);

	{
		#pragma HLS PIPELINE
		//#pragma HLS dataflow

		TraverseMatrix3(LocalMatIn, LocalMatOut);
		StreamDataOut(LocalMatOut, M_AXIS_OUT);

//		TraverseMatrix5(LocalMatIn1, LocalMatOut1);
//		StreamDataOut_1(LocalMatOut1, M_AXIS_OUT_1);
	}


	return;
}


void LoadData(hls::stream<pkt>& S_AXIS_IN, int* LocalMatIn) //, int* LocalMatIn1)
{
	int k = 0;
	pkt Input;

	loopInput:
	do
	{
		Input = S_AXIS_IN.read();

		LocalMatIn[k] = Input.data;
		//LocalMatIn1[k] = Input.data;
		k++;

	}while (Input.last == 0 );
}

void PreProcess(hls::stream<pkt>& S_AXIS_IN, hls::stream<pkt>& S_OUT)
{
	pkt Input;
	pkt Output;

	loopPreProcess:
	do
	{
		Input = S_AXIS_IN.read();
		Output.data = Input.data / 65535 * 255;
		Output.last = Input.last;
		S_OUT.write(Output);
	}while (Input.last == 0 );

}

int ApplyFilterDim3(int* LocalMatIn, int i, int j)
{
	int Filter[FILTER_SIZE_3][FILTER_SIZE_3] =  {{ 1, 0, -1 },
											     { 2, 0, -2 },
											     { 1, 0, -1 }};
	//#pragma HLS unroll


	int FilterOffset = FILTER_SIZE_3/2;
    LoopMat3:
	int sum = 0;
	for (int u = -FilterOffset; u <= FilterOffset; ++u)
	{
	    LoopMat4:
		for (int v = -FilterOffset; v <= FilterOffset; ++v)
		{

			if ((i+u >= 0) && (j+v >= 0) && (i+u < MATRIX_SIZE) && (j+v < MATRIX_SIZE))
			{
				//#pragma HLS PIPELINE
				//count += 2;
				int k = (i+u)*MaxMatSize + (j+v);
				sum += LocalMatIn[k] * Filter[u+FilterOffset][v+FilterOffset];
			}
		}
	}
	return sum / 9;
}



void TraverseMatrix3(int* LocalMatIn, int* LocalMatOut)
{
	#pragma HLS ARRAY_PARTITION variable = LocalMatIn dim = 1 complete
	#pragma HLS ARRAY_PARTITION variable = LocalMatOut dim = 1 complete

    LoopMat31:
	for (int i = 0 ; i < MaxMatSize; ++i)
	{
		//#pragma HLS unroll factor=8

	    LoopMat32:
		for (int j = 0 ; j < MaxMatSize; ++j)
		{
			#pragma HLS PIPELINE
			//#pragma HLS latency min=24 max=32
			LocalMatOut[i*MaxMatSize + j] = ApplyFilterDim3(LocalMatIn, i, j);
 		}
	}
}

void TraverseMatrix5(int* LocalMatIn, int* LocalMatOut)
{
    LoopMat51:
	for (int i = 0 ; i < MaxMatSize; ++i)
	{
	    LoopMat52:
		for (int j = 0 ; j < MaxMatSize; ++j)
		{
			LocalMatOut[i*MaxMatSize + j] = ApplyFilterDim5(LocalMatIn, i, j);
 		}
	}

}




int ApplyFilterDim5(int* LocalMatIn, int i, int j)
{
	int Filter[FILTER_SIZE_5][FILTER_SIZE_5] =  {{  1, 2, 3, 2, 1 },
												 {  1, 2, 3, 2, 1 },
												 {  0, 0, 0, 0, 0 },
												 { -1,-2,-3,-2,-1 },
												 { -1,-2,-3,-2,-1 },
												};

	//#pragma HLS unroll

	int FilterOffset = FILTER_SIZE_5/2;
    LoopMat3:
	int sum = 0;
	for (int u = -FilterOffset; u <= FilterOffset; ++u)
	{
        //#pragma HLS PIPELINE II=1

	    LoopMat4:
		for (int v = -FilterOffset; v <= FilterOffset; ++v)
		{

			if ((i+u >= 0) && (j+v >= 0) && (i+u < MATRIX_SIZE) && (j+v < MATRIX_SIZE))
			{
				//#pragma HLS PIPELINE
				//count += 2;
				int k = (i+u)*MaxMatSize + (j+v);
				sum += LocalMatIn[k] * Filter[u+FilterOffset][v+FilterOffset];
			}


		}
	}
	return sum / 9;
}


void StreamDataOut(int* LocalMatOut, hls::stream<pkt>& M_AXIS_OUT)
{
	loopOutput:
	for(int i=0; i< MaxMatSize * MaxMatSize; i++)
	{
 		pkt sout;
		sout.data = LocalMatOut[i];
		sout.last = (i==((MaxMatSize * MaxMatSize)-1))? 1 : 0;
		sout.strb = -1;
		sout.keep = 15;
		sout.user = 0;
		sout.id = 0;
		sout.dest = 0;
		M_AXIS_OUT.write(sout);
	}
}

void StreamDataOut_1(int* LocalMatOut, hls::stream<pkt>& M_AXIS_OUT)
{
	loopOutput:
	for(int i=0; i< MaxMatSize * MaxMatSize; i++)
	{
 		pkt sout;
		sout.data = LocalMatOut[i];
		sout.last = (i==((MaxMatSize * MaxMatSize)-1))? 1 : 0;
		sout.strb = -1;
		sout.keep = 15;
		sout.user = 0;
		sout.id = 0;
		sout.dest = 0;
		M_AXIS_OUT.write(sout);
	}
}


