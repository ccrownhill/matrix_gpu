#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <chrono>
#include "benchmark.hpp"


using namespace std;

	const int width = 512, height = 512;
    double total_time = 0;




void generate_image(int n, float z_range){
    size_t SIZE = width * height * 3;		//3 is for the 3 colour channels; red, green, blue
	unsigned char* image_data = (unsigned char*) malloc(SIZE);	//Allocating memory for image of size w: 256 & h: 256 pixels & 3 colour channels
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
    double x = 0;
    double y = 0;
    double z = 0;
    std::vector<double> pixel_vec;

    int x_pixel = 0;
    int y_pixel = 0;

	begin = std::chrono::steady_clock::now();

	{ //Image Painter - sets the background and the diagonal line to the array
		for (int row = 0; row < height; row++) {
			for (int col = 0; col < width; col++) {
				
                x = ((double(col) / width) * 10) - 5;
                y = ((1 - (double(row) / height)) * 10) - 5;
                
                
                pixel_vec = pixel_val(x, y);
                x_pixel = int((pixel_vec[0] + 5) * (width - 1) / 10);
                y_pixel = int((pixel_vec[1] + 5) * (height - 1) / 10);
                pixel = x_pixel + y_pixel * width;
                z = pixel_vec[2];
                z+=z_range/2;
                z/=z_range;
               

			   
                uint32_t colour = pixel_col(z);
                if(pixel < ((width * height) -4) &&  pixel >= 0){
                    image_data[pixel*3] = (colour >> 16) & 0xFF; //red
                    image_data[pixel*3 + 1] = (colour >> 8) & 0xFF; //green
                    image_data[pixel*3 + 2] = colour & 0xFF; //blue
                }
				
				pixel++;
			}
		}
	}
	#if (testing_disp == 0)
		end = std::chrono::steady_clock::now();
	#endif
    
	{ //Image Body - outputs image_data array to the .ppm file, creating the image
		for (int x = 0; x < SIZE; x++) {
			int value = image_data[x];		//Sets value as an integer, not a character value
			myImage << value << " \n" ;		//Sets 3 bytes of colour to each pixel	
		}
	}

	#if (testing_disp == 1)
		end = std::chrono::steady_clock::now();
	#endif

    total_time += std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count();
	myImage.close();

	free(image_data);
	image_data = 0;
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