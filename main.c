#include <stdlib.h>
#include <stdio.h>
#include "cbmp.h"
#include <string.h>
#include <unistd.h>
#include <time.h>


unsigned char eroded_image[BMP_WIDTH][BMP_HEIGHT];
unsigned char removed_cells_image[BMP_WIDTH][BMP_HEIGHT];
unsigned char input_image[BMP_WIDTH][BMP_HEIGHT][BMP_CHANNELS];
unsigned char output_image[BMP_WIDTH][BMP_HEIGHT][BMP_CHANNELS];

unsigned int amount_of_cells = 0;
unsigned int erosion_happened;
unsigned int optimal_Threshold;

#define TIMER_start() clock_t start = clock();

#define TIMER_stop(name) do { \
    clock_t end = clock(); \
    double time_taken = ((double) (end - start)) / CLOCKS_PER_SEC; \
    printf("%stog: %f seconds\n", name, time_taken); \
} while(0)

typedef struct
{
  unsigned int x;
  unsigned int y;
} Coordinate;

Coordinate coordinates[1000];

static inline int max(int a, int b)
{
  return a > b ? a : b;
}

static inline int min(int a, int b)
{
  return a < b ? a : b;
}

void grey_scale(unsigned char rgb_image[BMP_WIDTH][BMP_HEIGHT][BMP_CHANNELS], unsigned char grey_scale_image[BMP_WIDTH][BMP_HEIGHT])
{
  int freq_array[256]={0};
  float bc_Variance;
  float prob1;
  float prob2;
  float mean1;
  float mean2;
  unsigned int total_pixels = BMP_HEIGHT*BMP_WIDTH;
  float findMaxBCVariance = 0.0;
  for (int x = 0; x < BMP_WIDTH; x++)
  {
    for (int y = 0; y < BMP_HEIGHT; y++)
    {
      unsigned char red = rgb_image[x][y][0];
      unsigned char green = rgb_image[x][y][1];
      unsigned char blue = rgb_image[x][y][2];

      unsigned char grey = (red + green + blue)/3;
      // binray threshhold kan laves her. så kan man slippe for division, og for at lagre den midlertidige resultat grey_scale_image i et array
      freq_array[grey]++;
      grey_scale_image[x][y] = grey;
    }
  }
  for(int i=0; i <= 255; i++)
      {
        float count1 = 0.0;
        float count2 = 0.0;
        int sum1 = 0;
        int sum2 = 0;

        for(int j =0; j <= 255; j++)
        {
          if(i >= j)
          {
            count1+= freq_array[j];
            sum1 += j *freq_array[j];
          }
          else
          {
            count2+= freq_array[j];
            sum2+= j*freq_array[j];
            }
        }
          prob1 = count1 / (float)total_pixels;
          prob2 = count2 / (float)total_pixels;
          mean1 = (float)sum1 / count1;
          mean2 = (float)sum2 / count2;
          bc_Variance = prob1 * prob2 * (mean1-mean2)*(mean1-mean2);

          if(bc_Variance > findMaxBCVariance)
          {
            findMaxBCVariance = bc_Variance;
            optimal_Threshold = i;
            
          }
      }
      printf("The optimal threshold is: %d\n", optimal_Threshold);
}

void binary_threshold(unsigned char grey_scale_image[BMP_WIDTH][BMP_HEIGHT], unsigned char black_white_image[BMP_WIDTH][BMP_HEIGHT])
{
  for (int x = 0; x < BMP_WIDTH; x++)
  {
    for (int y = 0; y < BMP_HEIGHT; y++)
    {
      
      if (grey_scale_image[x][y] <= optimal_Threshold)
      {
        black_white_image[x][y] = 0;
      }
      else
      {
        black_white_image[x][y] = 255;
      }
    }
  }
}

void convert_2d_to_3d(unsigned char grey_scale_image[BMP_WIDTH][BMP_HEIGHT], unsigned char rgb_image[BMP_WIDTH][BMP_HEIGHT][BMP_CHANNELS])
{
  for (int x = 0; x < BMP_WIDTH; x++)
  {
    for (int y = 0; y < BMP_HEIGHT; y++)
    {
      rgb_image[x][y][0] = grey_scale_image[x][y];
      rgb_image[x][y][1] = grey_scale_image[x][y];
      rgb_image[x][y][2] = grey_scale_image[x][y];
    }
  }
}

void erode(unsigned char black_white_image[BMP_WIDTH][BMP_HEIGHT], unsigned char eroded_image[BMP_WIDTH][BMP_HEIGHT])
{
  erosion_happened = 0;
  unsigned char erosion_structure[3][3] = {{0, 1, 0}, {1, 1, 1}, {0, 1, 0}};
  unsigned char temp_image[BMP_WIDTH][BMP_HEIGHT];
  char filename[50];

  for (int x = 0; x < BMP_WIDTH; x++)
  {
    for (int y = 0; y < BMP_HEIGHT; y++)
    {
      temp_image[x][y] = black_white_image[x][y];
    }
  }

  for (int x = 0; x < BMP_WIDTH; x++)
  {
    for (int y = 0; y < BMP_HEIGHT; y++)
    {
      unsigned char shouldErode = 0;
      for (signed char dx = -1; dx <= 1; dx++)
      {
        for (signed char dy = -1; dy <= 1; dy++)
        {
          if (erosion_structure[dx + 1][dy + 1] == 1 && black_white_image[x + dx][y + dy] == 0)
          {
            if (black_white_image[x][y] == 255)
            {
              shouldErode = 1;
              break;
            }
          }
        }
        if (shouldErode)
          break;
      }
      if (shouldErode)
      {
        temp_image[x][y] = 0;
        erosion_happened = 1;
      }
    }
  }

  for (int x = 0; x < BMP_WIDTH; x++)
  {
    for (int y = 0; y < BMP_HEIGHT; y++)
    {
      eroded_image[x][y] = temp_image[x][y];
    }
  }
}

void detect_cells(unsigned char eroded_image[BMP_WIDTH][BMP_HEIGHT], unsigned char removed_cells_image[BMP_WIDTH][BMP_HEIGHT])
{
  unsigned char cell_detected = 0;

  for (int x = 0; x < BMP_WIDTH; x++)
  {
    for (int y = 0; y < BMP_HEIGHT; y++)
    {
      if (eroded_image[x][y] == 255)
      {
        unsigned char white_in_inner_square = 0;
        unsigned char white_in_outer_square = 0;

        int min_dx = max(-6, -x);
        int max_dx = min(6, BMP_WIDTH - x - 1);
        int min_dy = max(-6, -y);
        int max_dy = min(6, BMP_HEIGHT - y - 1);

        for (int dx = min_dx; dx <= max_dx; dx++)
        {
          for (int dy = min_dy; dy <= max_dy; dy++)
          {
            int newX = x + dx;
            int newY = y + dy;

            if (eroded_image[newX][newY] == 255)
            {
              if (min_dx <= dx && dx <= max_dx && min_dy <= dy && dy <= max_dy)
              {
                if (abs(dx) <= 5 && abs(dy) <= 5)
                {
                  white_in_inner_square = 1;
                }
                if (abs(dx) == 6 || abs(dy) == 6)
                {
                  white_in_outer_square = 1;
                }
              }
            }
          }
        }

        if (white_in_inner_square == 1 && white_in_outer_square == 0)
        {
          amount_of_cells++;
          coordinates[amount_of_cells - 1].x = x;
          coordinates[amount_of_cells - 1].y = y;
          printf("Celle nummer %d har x-koordinatet %d og y-koordinatet %d\n", amount_of_cells, coordinates[amount_of_cells - 1].x, coordinates[amount_of_cells - 1].y);

          for (int dx = max(-5, -x); dx <= min(5, BMP_WIDTH - x - 1); dx++)
          {
            for (int dy = max(-5, -y); dy <= min(5, BMP_HEIGHT - y - 1); dy++)
            {
              int newX = x + dx;
              int newY = y + dy;
              eroded_image[newX][newY] = 0;
            }
          }
        }
      }
    }
  }
  for (int x = 0; x < BMP_WIDTH; x++)
  {
    for (int y = 0; y < BMP_HEIGHT; y++)
    {
      removed_cells_image[x][y] = eroded_image[x][y];
    }
  }
}

void insert_marks_at_cell_locations(unsigned char input_image[BMP_WIDTH][BMP_HEIGHT][BMP_CHANNELS])
{
  for (int i = 0; i < amount_of_cells; i++)
  {
    int base_x = coordinates[i].x;
    int base_y = coordinates[i].y;

    for (int dx = -3; dx < 4; dx++)
    {
      for (int dy = -3; dy < 4; dy++)
      {
        int x = base_x + dx;
        int y = base_y + dy;

        if (x >= 0 && x < BMP_WIDTH && y >= 0 && y < BMP_HEIGHT)
        {
          //if (dx == -6 || dx == 6 || dy == -6 || dy == 6)
          //{
            input_image[x][y][0] = 255;
            input_image[x][y][1] = 0;
            input_image[x][y][2] = 0;
          //}
        }
      }
    }
  }
}

void erode_and_detect_loop(unsigned char black_white_image[BMP_WIDTH][BMP_HEIGHT], char * output_file_path)
{
  int sleep_time = 1;
  //erode and print
  erode(black_white_image, eroded_image);
  
  if (erosion_happened)
  {
    //print the eroded image
    convert_2d_to_3d(eroded_image, output_image);
    write_bitmap(output_image, "output_images/LiveProcess.bmp");
    //sleep(sleep_time);

    //detect cells and print
    detect_cells(eroded_image, removed_cells_image);
    convert_2d_to_3d(removed_cells_image, output_image);
    write_bitmap(output_image, "output_images/LiveProcess.bmp");
    //sleep(sleep_time);

    //insert marks and print
    insert_marks_at_cell_locations(input_image);
    write_bitmap(input_image, "output_images/LiveProcess.bmp");
    //sleep(sleep_time);

    //Recurse
    erode_and_detect_loop(removed_cells_image, output_file_path);
  }
  else if (!erosion_happened)
  {
    insert_marks_at_cell_locations(input_image);
    write_bitmap(input_image, output_file_path);
    printf("Alle celler er opdaget!\n");
  }
}

unsigned char grey_image[BMP_WIDTH][BMP_HEIGHT];
unsigned char black_white_image[BMP_WIDTH][BMP_HEIGHT];
unsigned char eroded_image[BMP_WIDTH][BMP_HEIGHT];
unsigned char removed_cells_image[BMP_WIDTH][BMP_HEIGHT];

int main(int argc, char **argv)
{
TIMER_start();
  if (argc != 3)
  {
    fprintf(stderr, "Wrong main arguments. Use: %s <output file path> <output file path>\n", argv[0]);
    exit(1);
  }

  read_bitmap(argv[1], input_image);

  grey_scale(input_image, grey_image);

  binary_threshold(grey_image, black_white_image);

  /*
  while (erosion_happened)
  {
    erode(black_white_image, eroded_image);
    detect_cells(eroded_image, removed_cells_image);
    insert_marks_at_cell_locations(input_image);
    write_bit
    map(input_image, argv[2]);
  }
  */

  erode_and_detect_loop(black_white_image, argv[2]);

  // insert_marks_at_cell_locations(input_image);

  // convert_2d_to_3d(removed_cells_image, output_image);

  // write_bitmap(input_image, argv[2]);

  // printf("Done!\n");
  TIMER_stop("Programmet tog:");
  printf("Paa billedet var antallet af celler lig med: %d\n", amount_of_cells);
  return 0;
}