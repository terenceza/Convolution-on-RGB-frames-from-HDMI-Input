

//#include "ap_axi_sdata.h" // ap_axis can also be used, but it will include all sideband signals which we don't need
#include "hls_stream.h"
#include "ap_int.h"
#include "ap_axi_sdata.h"
#include "AccelConvFilter.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

//unsigned int ProcLinesCnt = 0;

// Convolute a full video frame with 2-D filter

void AccelConvFilter(hls::stream<grayPix>& S_AXIS_IN, hls::stream<rgbPix>& M_AXIS_OUT,
		 	 	 	 unsigned int* MatRowsP, unsigned int* MatColsP,
					 unsigned int* PassThroughP,
					 int* FilterInData, unsigned int* FilterDimX, unsigned int* FilterDimY)
//					 int* SetFilter,  int*  FiltMemInAddr, int* FiltSizeP)
{

    //#pragma HLS INTERFACE ap_ctrl_none port=return
	#pragma HLS INTERFACE mode=s_axilite bundle=CONTROL_BUS port=return

	#pragma HLS INTERFACE mode=axis port=S_AXIS_IN  name=IN_STREAM
	#pragma HLS INTERFACE mode=axis port=M_AXIS_OUT depth=1024 name=OUT_STREAM

	#pragma HLS INTERFACE mode=s_axilite port=PassThroughP name=PassThroughP

	//#pragma HLS INTERFACE mode=s_axilite 	  port=FiltSizeP 	 bundle=FiltProp
	//	// Ingress Matrix
	#pragma HLS INTERFACE mode=s_axilite port=MatRowsP  bundle=MatPropIn
	#pragma HLS INTERFACE mode=s_axilite port=MatColsP  bundle=MatPropIn

	// Filter In
	#pragma HLS INTERFACE mode=m_axi port=FilterInData offset=slave bundle=gmem
	#pragma HLS INTERFACE mode=s_axilite port=FilterInData
	#pragma HLS INTERFACE mode=s_axilite port=FilterDimX name=FilterDimX
	#pragma HLS INTERFACE mode=s_axilite port=FilterDimY name=FilterDimY

	unsigned int Pass = *PassThroughP;

	// no padding - store data for processing and send blacklines
	if (Pass == 1)
	{
		PassThrough(S_AXIS_IN, M_AXIS_OUT);
		return;
	}

	grayPix in;

	// used to handle ring buffer of 16 elements
	ap_uint<4> Head = 0;
	ap_uint<4> BuffTailIndex = 0;

	// store up to 16 lines of 720x1280 Gray pixels, large enough for a convolution with a 15x15 filter
	unsigned char LocalMatIn[MAX_NUM_PIX_PER_LINE * BUFFER_ARRAY_SIZE];
	//rgbPix LocalMatOut[MAX_NUM_PIX_PER_LINE * BUFFER_ARRAY_SIZE];

    unsigned int MatLines = std::min(*MatRowsP, MAX_NUM_LINES);
	unsigned int MatCols  = std::min(*MatColsP, MAX_NUM_PIX_PER_LINE);

	//printf("FilterSize = %d FilterOffset = %d \n", FilterSize, FilterSize/2 );

	unsigned int RealLineIndex = 0;
	ap_uint<4> BuffHeadIndex = (*FilterDimX)/2;

	bool IsLastPix = false;


	unsigned int FilterSize = (*FilterDimX) * (*FilterDimY);

	int Filter[128];
	//memcpy((char*)Filter, (char*)FilterInData, FilterSize*4);
	for (int i = 0; i < FilterSize; ++i)
	{
		#pragma HLS PIPELINE off
		Filter[i] = FilterInData[i];
	}

//	printf("Filter: ");
//
//	for (int i = 0 ; i < FilterSize ; ++i )
//	{
//		printf(" %d ", Filter[i]);
//	}




	//for (int i = 0 ; i < FilterSize; ++i)
	{
		StoreLineSendPix(S_AXIS_IN, LocalMatIn, BuffTailIndex, M_AXIS_OUT);
		++BuffTailIndex;
		++RealLineIndex;
		StoreLineSendPix(S_AXIS_IN, LocalMatIn, BuffTailIndex, M_AXIS_OUT);
		++BuffTailIndex;
		++RealLineIndex;
		StoreLineSendPix(S_AXIS_IN, LocalMatIn, BuffTailIndex, M_AXIS_OUT);
		++BuffTailIndex;
		++RealLineIndex;
	}

	// Here BuffHeadIndex is still the beginning of the buffer ready for processing

	Loop_A:
	//for (unsigned int RealLineIndex = 0 ; RealLineIndex < MAX_NUM_LINES; ++RealLineIndex)
	do
	{
		//#pragma HLS DATAFLOW
		//#pragma HLS PIPELINE off
		ProcLine(S_AXIS_IN, LocalMatIn, RealLineIndex, BuffHeadIndex, BuffTailIndex, M_AXIS_OUT, Filter, *FilterDimX, IsLastPix);		// Read only there is still room in LocalMatIn
		++RealLineIndex;
		++BuffHeadIndex;
		++BuffTailIndex;

	}while(RealLineIndex < MAX_NUM_LINES);
	//while(IsLastPix == false);

	//printf("ProcLinesCnt = %d \n ", ProcLinesCnt);

	return;
}


void ProcLine(hls::stream<grayPix>& S_AXIS_IN, unsigned char* LocalMatIn,
			  unsigned int RealLineIndex, ap_uint<4> BuffHeadIndex, ap_uint<4> BuffTailIndex,
			  hls::stream<rgbPix>& M_AXIS_OUT,
			  int* Filter, unsigned int FilterDim,
			  bool& IsLastPix)
{
	//printf("ProcLine %d \n", ArrayLine);


	// i, j are indices in linear buffer represented as a 2D array
	unsigned int i = BuffHeadIndex; // line in the buffer to be processed
	unsigned int j = 0;

	unsigned int Res  = 0;

	ap_uint<3> Stride = 0;

	// k is the index in the linear buffer to process
	unsigned int kproc = BuffHeadIndex * MAX_NUM_PIX_PER_LINE;
	unsigned int toproc = kproc + MAX_NUM_PIX_PER_LINE;

	// k is the index in the linear buffer to store
	unsigned int kstore = BuffTailIndex * MAX_NUM_PIX_PER_LINE;
	unsigned int tostore = kstore + MAX_NUM_PIX_PER_LINE;

	rgbPix out;
	grayPix in;

	unsigned int PixCol = 0;

	Loop_ProcLine:
	do
	{
		//#pragma HLS PIPELINE II=1
		// read and store the data
		S_AXIS_IN.read(in);
		LocalMatIn[kstore] = in.data;

		//printf("PixCol = %d FilterSize/2=%d  \n", PixCol, FilterSize/2);

		if (/*(Stride == 0) && */
			(PixCol >= FilterDim/2) && (PixCol < (MAX_NUM_PIX_PER_LINE - (FilterDim/2))))
		{
			Res = ApplyFilters(LocalMatIn, BuffHeadIndex, PixCol, MAX_NUM_PIX_PER_LINE, Filter, FilterDim);
			//printf("Res = %d \n ", Res);
			//Res = LocalMatIn[kstore];
			//Res = in.data;
			out.data =  Res | (Res << 8) | (Res << 16);

			//printf("out.data=%x \n", out.data);
		}
		else
		{
			out.data =  0x101010;
		}
		out.last = in.last;
		out.dest = in.dest;
		out.id   = in.id;
		out.keep = in.keep;
		out.strb = in.strb;
		out.user = in.user;

		//LocalMatOut[PixCol] = out;

		M_AXIS_OUT.write(out);
		//++ProcLinesCnt;

		++j;
		++PixCol;
		++kproc;
		++kstore;
		++Stride;

	}//while(in.last == 0);
	while(kstore < tostore);

	IsLastPix = (in.last == 1) ;

	//printf("\n Process BufferTail %d \n\n", BufferTail);

	//printf("\n BufferLineIndex = %d \n\n", BufferLineIndex);

}


unsigned int ApplyFilters(  unsigned char MatIn[], int i, int j, int PixPerLine,
							int* Filter, unsigned int FilterDim)
{
	//printf("PixPerLine = %d \n", PixPerLine);

	ap_int<32> FilterSize = ap_int<32>(FilterDim) * ap_int<32>(FilterDim);

	Bits8 ResX, ResY, Res;
	ap_int<32> SumX = 0;
	ap_int<32> SumY = 0;

	ap_int<32> ResultsX[] =  { 0,0,0,0,0,0,0,0,0 };
	ap_int<32> ResultsY[] =  { 0,0,0,0,0,0,0,0,0 };

	{
		//#pragma HLS DATAFLOW
		ApplyFilterDim3X(MatIn, i, j, PixPerLine, Filter, FilterDim, ResultsX);
		ApplyFilterDim3Y(MatIn, i, j, PixPerLine, Filter, FilterDim, ResultsY);
	}

	Loop_Sum:
	for (int i = 0 ; i < FilterSize ; ++i)
	{
		//printf(" %d + ", ResultsX[i]);
		SumX+=ResultsX[i];
		SumY+=ResultsY[i];
	}

	//printf("SumX = %d \n", (int)SumX);

	//ResY = ApplyFilterDim3Y(MatIn, i, j, PixPerLine, FilterSize);

	//Res  = static_cast<Bits8>(hls::sqrt(ResX*ResX + ResY*ResY) & 0xFF);

	if (SumX < 0) SumX = -SumX;
	if (SumY < 0) SumY = -SumY;

	SumX = ap_fixed<32,16>(SumX) / ap_fixed<32,16>(FilterSize);
	SumY = ap_fixed<32,16>(SumY) / ap_fixed<32,16>(FilterSize);

//	printf("GradX = %d \n",SumX);
//	printf("GradY = %d \n",SumY);


	//printf("Sum = %d \n", Sum);

	ap_fixed<32,16> SumX_fixed = ap_fixed<32,16>(SumX);
	ap_fixed<32,16> SumY_fixed = ap_fixed<32,16>(SumY);

	ap_int<32> Mag = (ap_int<32>)( hls::sqrt(SumX_fixed*SumX_fixed + SumY_fixed*SumY_fixed));

	//printf("Mag = %d \n", Mag);

	if (Mag > 255) Mag = 255;

	return Mag.to_uint();
}


void ApplyFilterDim3X( unsigned char MatIn[], int i, int j, int PixPerLine,
					   int* FilterX, unsigned int FilterDim,
					   ap_int<32>* Results)
{
	#pragma HLS ARRAY_PARTITION variable=Results type=complete

	int a = 0;

//	ap_int<32> FilterXX[9] =  {1, 0, -1 ,
//							  2, 0, -2 ,
//							  1, 0, -1 };

	ap_uint<4> ii = 0;
	int Offset = FilterDim/2;
	int u = -Offset;
	int v = -Offset;

	ApplyFilterDim3X:
	for (a = 0; a < FilterDim * FilterDim; ++a)
	{
		//#pragma HLS UNROLL
		//#pragma HLS PIPELINE II=1

		ii = i+u;
		int jj = (ii)*PixPerLine + (j+v);
		Results[a] = ap_int<32>(MatIn[jj]) * (ap_int<32>)FilterX[a];
//		printf("MatIn[%d] = %d FilterX[%d] = %d Result[%d] = %d \n",
//				jj, MatIn[jj],
//				a,  FilterX[a], a,
//				Results[a]);

		if (v == Offset)
		{
			v = -Offset;
			++u;
		}
		else
		{
			++v;
		}
	}

	//printf("\n\n");
}


void ApplyFilterDim3Y( unsigned char MatIn[], int i, int j, int PixPerLine,
					   int* FilterY, unsigned int FilterDim,
					   ap_int<32>* Results)
{
	#pragma HLS ARRAY_PARTITION variable=Results type=complete

	int a = 0;

	ap_int<32> FilterYY[9] =  {-1, -2, -1 ,
							   0, 0, 0 ,
							   1, 2, 1 };

	ap_uint<4> ii = 0;
	int Offset = FilterDim/2;
	int u = -Offset;
	int v = -Offset;

	ApplyFilterDim3Y:
	for (a = 0; a < FilterDim * FilterDim; ++a)
	{
		//#pragma HLS UNROLL
		//#pragma HLS PIPELINE II=1

		ii = i+u;
		int jj = (ii)*PixPerLine + (j+v);
		Results[a] = ap_int<32>(MatIn[jj]) * ((ap_int<32>)FilterYY[a]);
		//printf("Result[%d] = %d ", a, Results[a]);

		if (v == Offset)
		{
			v = -Offset;
			++u;
		}
		else
		{
			++v;
		}
	}

	//printf("\n\n");
}



void StoreLineSendPix(hls::stream<grayPix>& S_AXIS_IN,
					  unsigned char* LocalMatIn, ap_uint<4> BuffTailIndex,
					  hls::stream<rgbPix>& M_AXIS_OUT)
{
	grayPix in;
	rgbPix out;
	// k is the index in the linear buffer
	unsigned int k = BuffTailIndex * MAX_NUM_PIX_PER_LINE;
	unsigned int to = k + MAX_NUM_PIX_PER_LINE;

	Loop_StoreLine:
	do
	{
		// read and store the data
		S_AXIS_IN.read(in);
		LocalMatIn[k] = in.data;

		out.data =  0x000000;

		out.last = in.last;
		out.dest = in.dest;
		out.id   = in.id;
		out.keep = in.keep;
		out.strb = in.strb;
		out.user = in.user;

		M_AXIS_OUT.write(out);
		//++ProcLinesCnt;

		++k;
	}while(k < to);

	//printf("\n BufferLineIndex = %d \n\n", BufferLineIndex);

}

void PassThrough(hls::stream<grayPix>& S_AXIS_IN, hls::stream<rgbPix>& M_AXIS_OUT)
{
	grayPix in;
	rgbPix out;
	unsigned int data = 0;

	do
	{
		S_AXIS_IN.read(in);

		data = in.data;

		out.data = data | (data << 8) | (data << 16);
		out.last = in.last;
		out.dest = in.dest;
		out.id   = in.id;
		out.keep = in.keep;
		out.strb = in.strb;
		out.user = in.user;

		M_AXIS_OUT.write(out);

	}while(in.last == 0);


}

#if 0
// populate one line in the buffer
void ProcessLine(unsigned char* LocalMatIn, unsigned short LineIndex,
				 grayPix& tmp, bool IsLastLine, hls::stream<rgbPix>& M_AXIS_OUT)
{
	//printf("ProcessLine %d \n", LineIndex);

	rgbPix out;
	unsigned int k = LineIndex * MAX_NUM_PIX_PER_LINE;
	unsigned int to = k + MAX_NUM_PIX_PER_LINE;

	do
	{
		unsigned char val = LocalMatIn[k];
		out.data = val | 0x646464;
		out.last = 0;
		out.dest = tmp.dest;
		out.id   = tmp.id;
		out.keep = tmp.keep;
		out.strb = tmp.strb;
		out.user = tmp.user;

		if (IsLastLine && ( k == (MAX_NUM_PIX_PER_LINE - 1)) )
		{
			out.last = 1;
		}

		M_AXIS_OUT.write(out);
		++k;
	}while( k < to );
}


void DummyLine(grayPix& tmp, bool IsLastLine, hls::stream<rgbPix>& M_AXIS_OUT)
{
	//printf("ProcessLine %d \n", LineIndex);

	rgbPix out;
	unsigned int k = 0;
	k = 0;
	do
	{
		out.data = 0x646464;
		out.last = 0;
		out.dest = tmp.dest;
		out.id   = tmp.id;
		out.keep = tmp.keep;
		out.strb = tmp.strb;
		out.user = tmp.user;

		if (IsLastLine && ( k == (MAX_NUM_PIX_PER_LINE - 1)) )
		{
			out.last = 1;
		}

		M_AXIS_OUT.write(out);
		++k;
	}while( k < MAX_NUM_PIX_PER_LINE );
}

#endif

#if 0
void LoadFilters(INT8* FilterBuff, INT8* FilterX, INT8* FilterY, UINT8 FilterSize)
{
	int k = 0;

	for (int i = 0; i < FilterSize; ++i)
	{
		for (int j = 0; j < FilterSize; ++j)
        {
            FilterX[i][j] = FilterBuff[k];
            FilterY[j][i] = FilterBuff[k];
			++k;
        }
	}
}



void StreamDataOut(unsigned char* LocalMatOut, int NumPixels, hls::stream<grayPix>& M_AXIS_OUT, bool ReadLastPixel)
{
	grayPix Video_out;

	loopOutput:
	for(int i=0; i< NumPixels; i++)
	{
		Video_out.data = LocalMatOut[i];
		Video_out.last = 0;

		//Video_out.strb = -1;
		Video_out.keep = 1;
		Video_out.user = 0;
		//sout.id = 0;

		//sout.dest = 0;

		if ((i == NumPixels-1) && (ReadLastPixel == true))
		{
			Video_out.last = 1;
		}

		M_AXIS_OUT.write(Video_out);
	}
}

#endif
#if 0
Bits8 ApplyFilterDim3X( unsigned char MatIn[], int i, int j, int PixPerLine,
						unsigned char FilterSize,
						int Results[])
{
	#pragma HLS ARRAY_PARTITION variable=MatIn type=complete
	#pragma HLS ARRAY_PARTITION variable=Results type=complete

	int Sum = 0;

	ap_int<8> FilterX[3][3] =  {{ 1, 0, -1 },
								{ 2, 0, -2 },
								{ 1, 0, -1 }};


//	int kk = 0;
//	int SumArr[9];

    //#pragma HLS ARRAY_PARTITION variable=SumArr type=complete

//	ap_int<8> FilterX[5][5] =  {{ 2,  2,  4,  2, 2 },
//						  { 1,  1,  2,  1, 1 },
//						  { 0,  0,  0,  0,  0 },
//						  {-1, -1, -2, -1, -1 },
//						  {-2, -2, -4, -2, -2 }
//						 };

    //printf("\n ApplyFilterDim3X i = %d j = %d \n", i, j);

	unsigned char FilterOffset = FilterSize / 2;
	// used to find the i index of the neighbors in the buffer accounting for ring buffer
	ap_uint<4> ii = 0;

	#pragma HLS UNROLL

	Loop_ApplyFilterDim3X:
	for (int u = -FilterOffset; u <= FilterOffset; ++u)
	{
		#pragma HLS PIPELINE off
		#pragma HLS UNROLL factor=3

		ApplyFilterDim3X1:
		for (int v = -FilterOffset; v <= FilterOffset; ++v)
		{
			#pragma HLS PIPELINE off
			//#pragma HLS UNROLL factor=3

			// if i is line 15 in the buffer, neighboring lines are in line 0, 1 etc depending on filter size
			ii = i + u;
			int k = (ii)*PixPerLine + (j+v);

			//Sum += MatIn[k] * FilterX[u+FilterOffset][v+FilterOffset];
			Results[(u+1)*3+v+1] = MatIn[k] * FilterX[u+FilterOffset][v+FilterOffset];

			//printf("MatIn[%d] = %d  ")
			//printf("ApplyFilterDim3X i=%d j=%d u=%d v=%d j+v=%d ii=%d Sum=%d \n", i, j, u, v, j+v, ii&0xF, Sum);
		}
	}

//	for (int i = 0 ; i < 9 ; ++i)
//	{
//		Sum+= SumArr[i];
//	}
	//return static_cast<unsigned char>(hls::divide(Sum, (int)(FilterSize * FilterSize)));
	//return static_cast<Bits8>(Sum / (int)(FilterSize * FilterSize));
	return 1;

}


Bits8 ApplyFilterDim3Y( unsigned char MatIn[], int i, int j, int PixCols,
								unsigned char FilterSize)
{
	int Sum = 0;

	int FilterY[FILTER_SIZE_3][FILTER_SIZE_3] =  {{ -1, -2, -1 },
											      {  0,  0,  0 },
											      {  1,  2,  1 }};

//	int FilterY[5][5] =  {{ 2,  1,  0, -1, -2 },
//						  { 2,  1,  0, -1, -2 },
//						  { 4,  2,  0, -2, -4 },
//						  { 2,  1,  0, -1, -2 },
//						  { 2,  1,  0, -1, -2 }
//						 };

	unsigned char FilterOffset = FilterSize / 2;
	// used to find the i index of the neighbors in the buffer accounting for ring buffer
	ap_uint<4> ii = 0;

	LoopApplyFilterDim3Y:
	for (int u = -FilterOffset; u <= FilterOffset; ++u)
	{
		#pragma HLS PIPELINE off
		#pragma HLS UNROLL factor=3

		ApplyFilterDim3Y1:
		for (int v = -FilterOffset; v <= FilterOffset; ++v)
		{
			#pragma HLS PIPELINE off

			// if i is line 15 in the buffer, neighboring lines are in line 0, 1 etc depending on filter size
			ii = i + u;
			int k = (ii)*PixCols + (j+v);
			Sum += MatIn[k] * FilterY[u+FilterOffset][v+FilterOffset];

			//printf("MatIn[%d] = %d  ")
			//printf("ApplyFilterDim3X i=%d j=%d u=%d v=%d j+v=%d ii=%d Sum=%d \n", i, j, u, v, j+v, ii&0xF, Sum);
		}
	}

	return static_cast<unsigned char>(hls::divide(Sum, (int)(FilterSize * FilterSize)));
	//return static_cast<unsigned char>(Sum / (int)(FilterSize * FilterSize));

}
#endif

