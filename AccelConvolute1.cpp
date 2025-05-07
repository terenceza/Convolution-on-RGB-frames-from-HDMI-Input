

#include "AccelConvolute1.h"

void AccelConvolute1(hls::stream<rgbPixel>& S_AXIS_IN, hls::stream<grayPixel>& M_AXIS_OUT,
					 unsigned int* CHANNEL_ID)
{

    #pragma HLS INTERFACE mode=s_axilite bundle=CONTROL_BUS port=return
	#pragma HLS INTERFACE mode=axis port=S_AXIS_IN  name=IN_STREAM depth=1024
	#pragma HLS INTERFACE mode=axis port=M_AXIS_OUT name=OUT_STREAM depth=1024
	#pragma HLS INTERFACE mode=s_axilite port=CHANNEL_ID name=CHANNEL_ID

	// local variables
	rgbPixel video_in;
	grayPixel video_out;

	int PixCount = 0;
    int Stride = 4;
    unsigned int ChannelID = *CHANNEL_ID;

	// Apply grayscale conversion weights
	ap_ufixed<16,8> weight_r = 0.2126;
	ap_ufixed<16,8> weight_g = 0.7152;
	ap_ufixed<16,8> weight_b = 0.0722;

	Loop1:
	do
	{
		#pragma HLS pipeline off

		S_AXIS_IN.read(video_in);

		//unsigned int Data = video_in.get_data();

		//if (PixCount == 0)
		{
			UINT24 Data_in = video_in.data;

			//printf("Data = %6x \n", Data);

			ap_uint<8> B =    Data_in & 0xFF;
			ap_uint<8> G =   (Data_in >> 8 ) & 0xFF;
			ap_uint<8> R =   (Data_in >> 16) & 0xFF;

		    ap_ufixed<16,8> r = (ap_ufixed<16,8>)R;
			ap_ufixed<16,8> g = (ap_ufixed<16,8>)G;
			ap_ufixed<16,8> b = (ap_ufixed<16,8>)B;

			// Calculate grayscale value
			ap_ufixed<16,8> gray_value = weight_r * r + weight_g * g + weight_b * b;

			ap_uint<8> gray = (ap_uint<8>)gray_value;

			//printf("Y = %x %x \n", Y, Y.to_float());

			//printf("Gray = %x \n", Gray.to_uint());

			//UINT24 Data_out = Gray.to_uchar() | (Gray.to_uchar() << 8) | (Gray.to_uchar() <<16);
			//printf("Gray = %x Data_out = %6x \n", Gray.to_uint(), Data_out.to_uint());
//
//			UINT24 rgbData_out =   (ChannelID == 1) ? (R.to_uint() << 16) :
//								   (ChannelID == 2) ? (G.to_uint() << 8):
//								   (ChannelID == 3) ? (B.to_uint()) :
//								   (gray | (gray << 8) | (gray << 16));


			video_out.data = gray;
			video_out.last = video_in.last;
			video_out.keep = video_in.keep;
			video_out.strb = video_in.strb;
			video_out.user = video_in.user;
			video_out.id   = video_in.id;
			video_out.dest = video_in.dest;

			M_AXIS_OUT.write(video_out);
		}

//
//		if (++PixCount == Stride)
//		{
//			PixCount = 0;
//		}

	} while (video_in.last != 1);

	return;
}

