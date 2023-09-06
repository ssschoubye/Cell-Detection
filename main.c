#include <stdlib.h>
#include <stdio.h>
#include "cbmp.h"

//Function to invert pixels of an image (negative)
void invert(unsigned char input_image[BMP_WIDTH][BMP_HEIGTH][BMP_CHANNELS], unsigned char output_image[BMP_WIDTH][BMP_HEIGTH][BMP_CHANNELS]){
  for (int x = 0; x < BMP_WIDTH; x++)
  {
    for (int y = 0; y < BMP_HEIGTH; y++)
    {
      for (int c = 0; c < BMP_CHANNELS; c++)
      {
      output_image[x][y][c] = 255 - input_image[x][y][c];
      }
    }
  }
}

void grey_scale(unsigned char rgb_image[BMP_WIDTH][BMP_HEIGTH][BMP_CHANNELS], unsigned char grey_scale_image[BMP_WIDTH][BMP_HEIGTH]){
  for (int x=0; x < BMP_WIDTH; x++){
    for (int y=0; y < BMP_HEIGTH; y++){
      unsigned char red = rgb_image[x][y][0];
      unsigned char green = rgb_image[x][y][1];
      unsigned char blue = rgb_image[x][y][2];

      unsigned char grey = (red+green+blue)/3;

      grey_scale_image[x][y] = grey;
    }
  }
}

void convert_2d_to_3d(unsigned char grey_scale_image[BMP_WIDTH][BMP_HEIGTH], unsigned char rgb_image[BMP_WIDTH][BMP_HEIGTH][BMP_CHANNELS]){
  for(int x=0;x < BMP_WIDTH;x++){
    for(int y=0; y < BMP_HEIGTH; y++){
      rgb_image[x][y][0]=grey_scale_image[x][y];
      rgb_image[x][y][1]=grey_scale_image[x][y];
      rgb_image[x][y][2]=grey_scale_image[x][y];
    }
  }
}

  //Declaring the array to store the image (unsigned char = unsigned 8 bit)
  unsigned char input_image[BMP_WIDTH][BMP_HEIGTH][BMP_CHANNELS];
  unsigned char output_image[BMP_WIDTH][BMP_HEIGTH][BMP_CHANNELS];
  unsigned char grey_image[BMP_WIDTH][BMP_HEIGTH];

//Main function
int main(int argc, char** argv){
  //argc counts how may arguments are passed
  //argv[0] is a string with the name of the program
  //argv[1] is the first command line argument (input image)
  //argv[2] is the second command line argument (output image)

  //Checking that 2 arguments are passed
  if (argc != 3)
  {
      fprintf(stderr, "Usage: %s <output file path> <output file path>\n", argv[0]);
      exit(1);
  }


  printf("Example program - 02132 - A1\n");

  //Load image from file
  read_bitmap(argv[1], input_image);

  grey_scale(input_image, grey_image);

  convert_2d_to_3d(grey_image, output_image);

  //Save image to file
  write_bitmap(output_image, argv[2]);

  printf("Done!\n");
  return 0;
}