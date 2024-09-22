#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <chrono>
#include "benchmark_raw.hpp"


using namespace std;

	const int width = 512, height = 512;
    double total_time = 0;


float pixel_calc(float x, float y) {
	float z;

	//z =  1/std::sqrt(1+x*x); //benchmark num 1
    //z =  sin(x); //benchmark num 2
    //z =  x+y; //benchmark num 3
    //z =  std::sqrt(x*x+y*y); //benchmark num 4
    //z =  sin(x/y)+cos(x/y); //benchmark num 5
    z =  x*y+x*y*x*y+x*y*x*y*x*y; //benchmark num 6
	return z;
}

void generate_image(int n, float z_range){
    size_t SIZE = width * height * 3;		//3 is for the 3 colour channels; red, green, blue
	unsigned char* image_data = (unsigned char*) malloc(SIZE);	//Allocating memory for image of size w: 256 & h: 256 pixels & 3 colour channels
	float* raw_values = (float*) malloc(SIZE);	//Allocating memory for image of size w: 256 & h: 256 pixels & 3 colour channels
	memset(image_data, 255, SIZE);		//Setting all allocated memory to value of 255 (ie. white), this will be first 'painted' to image in the Body

	ofstream myImage;		//output stream object
    std::string filename = "image_" + std::to_string(2) + ".ppm";
	myImage.open(filename);

	std::chrono::steady_clock::time_point begin;
	std::chrono::steady_clock::time_point end;
	if (myImage.fail())
	{
		cout << "Unable to create image.ppm" << endl;

	}


	{ //Image header - Need this to start the image properties
		myImage << "P3" << endl;						//Declare that you want to use ASCII colour values
		myImage << width << " " << height << endl;		//Declare w & h
		myImage << "255" << endl;						//Declare max colour ID
	}

	int pixel = 0;
    float x = 0;
    float y = 0;
    float z = 0;

    int x_pixel = 0;
    int y_pixel = 0;

	begin = std::chrono::steady_clock::now();

	{ //Image Painter - sets the background and the diagonal line to the array
		for (int row = 0; row < height; row++) {
			for (int col = 0; col < width; col++) {
				
                x = ((double(col) / width) * 10) - 5;
                y = ((1 - (double(row) / height)) * 10) - 5;
                

                z = pixel_calc(x,y);
                z+=z_range/2;
                z/=z_range;
               	raw_values[pixel] = z;
				
				pixel++;
			}
		}
	}
	#if (testing_disp == 0)
		end = std::chrono::steady_clock::now();
	#endif
    
	{ //Image Body - outputs image_data array to the .ppm file, creating the image
		for(int i = 0; i < 512*512-1; i++){
			uint32_t colour = pixel_col(z);
			image_data[i*3] = (colour >> 16) & 0xFF; //red
			image_data[i*3 + 1] = (colour >> 8) & 0xFF; //green
			image_data[i*3 + 2] = colour & 0xFF; //blue
		}
	}




		for (int x = 0; x < SIZE; x++) {
			int value = image_data[x];		//Sets value as an integer, not a character value
			myImage << value << " \n" ;		//Sets 3 bytes of colour to each pixel	
		}

	#if (testing_disp == 1)
		end = std::chrono::steady_clock::now();
	#endif
    total_time += std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count();
	myImage.close();

	//free(image_data);
	//free(raw_values);
	//image_data = 0;
}

int main() {
	std::cout << "testing disp is " << testing_disp << "\n";
	float z_range_input;
	std::cout << "please enter the z_range_input\n";
	std::cin >> z_range_input;

    for (int i = 0; i < image_num; i++){
        generate_image(i, z_range_input);
    }
    std::cout << "Average time: " << total_time/image_num << std::endl;


	return 0;
}