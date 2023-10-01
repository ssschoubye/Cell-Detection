#include <stdlib.h>
#include <stdio.h>
#include "cbmp.h"
#include <string.h>
#include <time.h>


clock_t start, end;
double cpu_time_used;

unsigned char *eroded_image;
unsigned char *removed_cells_image;
unsigned char *black_white_image;
unsigned char *grey_image;

unsigned int amount_of_cells = 0;
unsigned int erosion_happened = 0;
unsigned int optimal_Threshold;
// unsigned int coordinate_index = 0;

//Macro brugt til at analyse time execution.

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

static inline int findMax(int a, int b)
{
  return a > b ? a : b;
}

static inline int findMin(int a, int b)
{
  return a < b ? a : b;
}


void grey_scale(unsigned char rgb_image[BMP_WIDTH][BMP_HEIGHT][BMP_CHANNELS])
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

      unsigned char grey = (red + green + blue) / 3;
      // binray threshhold kan laves her. så kan man slippe for division, og for at lagre den midlertidige resultat grey_scale_image i et array
      
      freq_array[grey]++;
      grey_image[y * BMP_WIDTH + x] = grey;
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
      
}

void binary_threshold(unsigned char *black_white_image)
{
  printf("Den optimale threshold er : %u/n", optimal_Threshold);
  for (int x = 0; x < BMP_WIDTH; x++)
  {
    for (int y = 0; y < BMP_HEIGHT; y++)
    {
      unsigned char threshold = 90;
      if (grey_image[y * BMP_WIDTH + x]  <= threshold)
      {
        black_white_image[y * BMP_WIDTH + x] = 0;
      }
      else
      {
        black_white_image[y * BMP_WIDTH + x] = 255;
      }
    }
  }
}

void erode(unsigned char *black_white_image, unsigned char *eroded_image)
{
  unsigned char erosion_done = 0;
  unsigned char erosion_structure[3][3] = {{0, 1, 0}, {1, 1, 1}, {0, 1, 0}};
  //unsigned char temp_image[BMP_WIDTH][BMP_HEIGHT];

  unsigned char *temp_image = (unsigned char *) malloc(BMP_HEIGHT * BMP_WIDTH);
  printf("Allocated %zu bytes at address %p\n", BMP_HEIGHT * BMP_WIDTH, (void *) temp_image);

  for (int x = 0; x < BMP_WIDTH; x++)
  {
    for (int y = 0; y < BMP_HEIGHT; y++)
    {
      temp_image[y * BMP_WIDTH + x] = black_white_image[y * BMP_WIDTH + x];
    }
  }

  for (int x = 1; x < BMP_WIDTH - 1; x++)
  {
    for (int y = 1; y < BMP_HEIGHT - 1; y++)
    {
      unsigned char shouldErode = 0;
      for (signed char dx = -1; dx <= 1; dx++)
      {
        for (signed char dy = -1; dy <= 1; dy++)
        {
          if (erosion_structure[dx + 1][dy + 1] == 1 && black_white_image[(y+dy)*BMP_WIDTH + (x + dx)] == 0)
          {
            if (black_white_image[y * BMP_WIDTH + x] == 255)
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
        temp_image[y * BMP_WIDTH + x] = 0;
        erosion_done = 1;
      }
    }
  }

  for (int x = 0; x < BMP_WIDTH; x++)
  {
    for (int y = 0; y < BMP_HEIGHT; y++)
    {
      eroded_image[y * BMP_WIDTH + x] = temp_image[y * BMP_WIDTH + x];
    }
  }
  erosion_happened = erosion_done;
  free(temp_image);
  printf("Temp image is no longer needed!");
}

void detect_cells(unsigned char *eroded_image, unsigned char *removed_cells_image)
{
  unsigned char cell_detected = 0;

  for (int x = 0; x < BMP_WIDTH; x++)
  {
    for (int y = 0; y < BMP_HEIGHT; y++)
    {
      if (eroded_image[y * BMP_WIDTH + x] == 255)
      {
        unsigned char white_in_inner_square = 0;
        unsigned char white_in_outer_square = 0;

        int findMin_dx = findMax(-6, -x);
        int findMax_dx = findMin(6, BMP_WIDTH - x - 1);
        int findMin_dy = findMax(-6, -y);
        int findMax_dy = findMin(6, BMP_HEIGHT - y - 1);

        for (int dx = findMin_dx; dx <= findMax_dx; dx++)
        {
          for (int dy = findMin_dy; dy <= findMax_dy; dy++)
          {
            int newX = x + dx;
            int newY = y + dy;

            if (eroded_image[newY * BMP_WIDTH + newX] == 255)
            {
              if (findMin_dx <= dx && dx <= findMax_dx && findMin_dy <= dy && dy <= findMax_dy)
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
          //printf("Celle nummer %d har x-koordinatet %d og y-koordinatet %d\n", amount_of_cells, coordinates[amount_of_cells - 1].x, coordinates[amount_of_cells - 1].y);

          for (int dx = findMax(-5, -x); dx <= findMin(5, BMP_WIDTH - x - 1); dx++)
          {
            for (int dy = findMax(-5, -y); dy <= findMin(5, BMP_HEIGHT - y - 1); dy++)
            {
              int newX = x + dx;
              int newY = y + dy;
              eroded_image[newY * BMP_WIDTH + newX] = 0;
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
      removed_cells_image[y * BMP_WIDTH + x] = eroded_image[y * BMP_WIDTH + x];
    }
  }
}

void erode_and_detect_loop(unsigned char *black_white_image)
{
  erode(black_white_image, eroded_image);
  if (erosion_happened)
  {
    detect_cells(eroded_image, removed_cells_image);
    erode_and_detect_loop(removed_cells_image);
  }
  else if (!erosion_happened)
  {
    printf("Alle celler er opdaget!\n");
    
  }
}

void insert_marks_at_cell_locations(unsigned char input_image[BMP_WIDTH][BMP_HEIGHT][BMP_CHANNELS])
{
  for (int i = 0; i < amount_of_cells; i++)
  {
    for (int dx = -8; dx < 9; dx++)
    {
        int x = coordinates[i].x + dx;
        int y = coordinates[i].y;
        if (x >= 0 && x < BMP_WIDTH && y >= 0 && y < BMP_HEIGHT)
        {
            input_image[x][y][0] = 255;
            input_image[x][y][1] = 0;
            input_image[x][y][2] = 0;
        }
    }
    for (int dy = -8; dy < 9; dy++)
      {
        int x = coordinates[i].x;
        int y = coordinates[i].y + dy;
        if (x >= 0 && x < BMP_WIDTH && y >= 0 && y < BMP_HEIGHT)
        {
            input_image[x][y][0] = 255;
            input_image[x][y][1] = 0;
            input_image[x][y][2] = 0;
          
        }
      }
  }
}

void convert_2d_to_3d(unsigned char rgb_image[BMP_WIDTH][BMP_HEIGHT][BMP_CHANNELS])
{
  for (int x = 0; x < BMP_WIDTH; x++)
  {
    for (int y = 0; y < BMP_HEIGHT; y++)
    {
      rgb_image[x][y][0] = grey_image[y * BMP_WIDTH + x] ;
      rgb_image[x][y][1] = grey_image[y * BMP_WIDTH + x] ;
      rgb_image[x][y][2] = grey_image[y * BMP_WIDTH + x] ;
    }
  }
}

unsigned char input_image[BMP_WIDTH][BMP_HEIGHT][BMP_CHANNELS];
unsigned char output_image[BMP_WIDTH][BMP_HEIGHT][BMP_CHANNELS];



int main(int argc, char **argv)
{
  
  
  //Af en eller anden grund kan der ikke køre mere end én timer ad gangen. Så afkommentere den funktion man nu gerne vil teste.
  start = clock();

  if (argc != 3)
  {
    fprintf(stderr, "Wrong main arguments. Use: %s <output file path> <output file path>\n", argv[0]);
    exit(1);
  }
  //TIMER_start();
  read_bitmap(argv[1], input_image);
  //TIMER_stop("Indlæsning af bitmap ");
  grey_image = (unsigned char *) malloc(BMP_HEIGHT * BMP_WIDTH);
  printf("Grey image allokeret %zu bytes at address %p\n", BMP_HEIGHT * BMP_WIDTH, (void *) black_white_image);
  //TIMER_start();
  grey_scale(input_image);
  
  //TIMER_stop("Grey Scale ");
  black_white_image = (unsigned char *) malloc(BMP_HEIGHT * BMP_WIDTH);
  printf("Black and White image allokeret %zu bytes at address %p\n", BMP_HEIGHT * BMP_WIDTH, (void *) black_white_image);
  //TIMER_start();
  binary_threshold(black_white_image);
  free(grey_image);
  printf("Grey image is free! \n");
  //TIMER_stop("Binary Threshold ");
  eroded_image = (unsigned char *) malloc(BMP_HEIGHT * BMP_WIDTH);
  printf("Eroded Image allokeret %zu bytes at address %p\n", BMP_HEIGHT * BMP_WIDTH, (void *) eroded_image);
  removed_cells_image = (unsigned char *) malloc(BMP_HEIGHT * BMP_WIDTH);
  printf("Remove Cells allokeret %zu bytes at address %p\n", BMP_HEIGHT * BMP_WIDTH, (void *) removed_cells_image);
  
  //TIMER_start();
  erode_and_detect_loop(black_white_image);
  //TIMER_stop("Erosion og detection ");
  free(eroded_image);
  printf("Eroded image is free \n");
  free(removed_cells_image);
  printf("Removed cells is free! \n");
  free(black_white_image);
  printf("Black and white is free! \n");
  //convert_2d_to_3d(removed_cells_image, output_image);

  //TIMER_start();
  insert_marks_at_cell_locations(input_image);
  //TIMER_stop("Indsætning af markering ");
  
  write_bitmap(input_image, argv[2]);
  


  // printf("Done!\n");
  printf("På billedet var antallet af celler lig med: %d\n", amount_of_cells);
  end = clock();
  cpu_time_used = end - start;
  printf("Total time: %f sec\n", cpu_time_used/CLOCKS_PER_SEC);

  return 0;
}

