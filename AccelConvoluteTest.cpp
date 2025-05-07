#include <stdio.h>
#include <stdlib.h>
#include "AccelConvolute1.h"

#define NUM_OF_ELEMENTS 64

int main(int argc, char** argv)
{

	ap_ufixed<16,8> weight_r = 0.2126;
	ap_ufixed<16,8> weight_g = 0.7152;
	ap_ufixed<16,8> weight_b = 0.0722;

	ap_uint<8> R =  0x12;
	ap_uint<8> G =  0x34;
	ap_uint<8> B =  0x45;

    ap_ufixed<16,8> r = (ap_ufixed<16,8>)R;
	ap_ufixed<16,8> g = (ap_ufixed<16,8>)G;
	ap_ufixed<16,8> b = (ap_ufixed<16,8>)B;

	// Calculate grayscale value
	ap_ufixed<16,8> gray_value = weight_r * r + weight_g * g + weight_b * b;
	ap_uint<8> gray = (ap_uint<8>)gray_value;

	printf("Gray scale value is %x \n", gray);

	hls::stream<rgbPixel> sin;
	hls::stream<grayPixel> sout;
	unsigned int ChannelID = 0;
	int x;
	//unsigned char r = 0;
	std::cout << "Input : " << std::endl;

	for (UINT32 i = 0 ; i < NUM_OF_ELEMENTS ; ++i)
	{
		rgbPixel Input;

		r = rand() % 255;
		x = Input.data =  (rand() % 255) | ((rand() % 255) << 8) | ((rand() % 255) << 16);

		Input.last = ( i == (NUM_OF_ELEMENTS - 1)) ? 1 : 0;
		sin.write(Input);

		if (i%8 == 0)
		{
			std::cout << std::endl;
		}

		printf(" %x ", x);

	}

	AccelConvolute1(sin, sout, &ChannelID);

	std::cout << "Output : " << std::endl;


	for (int i = 0 ; i < NUM_OF_ELEMENTS ; ++i)
	{
		if (i%8 == 0)
		{
			std::cout << std::endl;
		}

		printf(" %x ", sout.read().data);

	}


    return (EXIT_SUCCESS);
}
