#include <stdlib.h>
#include <stdio.h>
#include "cbmp.h"
#include <string.h>
#include <time.h>

clock_t start, end;
double cpu_time_used;

unsigned char *eroded_image;
unsigned char removed_cells_image[BMP_WIDTH][BMP_HEIGHT];

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

void grey_scale(unsigned char rgb_image[BMP_WIDTH][BMP_HEIGHT][BMP_CHANNELS], unsigned char grey_scale_image[BMP_WIDTH][BMP_HEIGHT])
{
  int freq_array[256]={0};
  float bc_Variance;
  float prob1;
  float prob2;
  float mean1;
  float mean2;
  unsigned int total_pixels = BMP_HEIGHT*BMP_WIDTH;
  float maxBCVariance = 0.0;
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

          if(bc_Variance > maxBCVariance)
          {
            maxBCVariance = bc_Variance;
            optimal_Threshold = i;
          }
      }
      
}

void binary_threshold(unsigned char grey_scale_image[BMP_WIDTH][BMP_HEIGHT], unsigned char black_white_image[BMP_WIDTH][BMP_HEIGHT])
{
  printf("Den optimale threshold er : %u/n", optimal_Threshold);
  for (int x = 0; x < BMP_WIDTH; x++)
  {
    for (int y = 0; y < BMP_HEIGHT; y++)
    {
      unsigned char threshold = 90;
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

void erode(unsigned char black_white_image[BMP_WIDTH][BMP_HEIGHT], unsigned char *eroded_image)
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
      temp_image[y * BMP_WIDTH + x] = black_white_image[x][y];
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
}

void detect_cells(unsigned char *eroded_image, unsigned char removed_cells_image[BMP_WIDTH][BMP_HEIGHT])
{
  unsigned char cell_detected = 0;
  unsigned char padded_image[BMP_WIDTH + 12][BMP_HEIGHT + 12];
  memset(padded_image, 0, sizeof(padded_image)); // fylder padded_image med 0'er

  for (int x = 6; x < BMP_WIDTH; x++)
  {
    for (int y = 6; y < BMP_HEIGHT; y++)
    {
      padded_image[x][y] = eroded_image[y * BMP_WIDTH + x];
    }
  }

  for (int x = 5; x <= BMP_WIDTH - 5; x++)
  {
    for (int y = 5; y <= BMP_HEIGHT - 5; y++)
    {
      unsigned char white_in_inner_square = 0;
      unsigned char white_in_outer_square = 0;
      for (signed char dx = -6; dx < 6; dx++)
      {
        for (signed char dy = -6; dy < 6; dy++)
        {
          if (padded_image[x + dx][y + dy] == 255)
          {
            // Tjek den indre og ydre firkant
            if (-5 <= dx && dx <= 4 && -5 <= dy && dy <= 4)
            {
              white_in_inner_square = 1;
            }
            if (dx == -6 || dx == 5 || dy == -6 || dy == 5)
            {
              white_in_outer_square = 1;
            }
          }
        }
        if (white_in_inner_square == 1 && white_in_outer_square == 0)
        {
          cell_detected = 1;
        }
        else
        {
          cell_detected = 0;
        }
      }
      // Farv den indre firkant sort, hvis en celle bliver fundet
      if (cell_detected)
      {
        coordinates[amount_of_cells].x = x;
        coordinates[amount_of_cells].y = y;
        printf("Celle nummer %d har x-koordinatet %d og y-koordinatet %d\n", amount_of_cells + 1, coordinates[amount_of_cells].x, coordinates[amount_of_cells].y);
        amount_of_cells++;
        // coordinate_index++;
        for (signed char dx = -5; dx < 5; dx++)
        {
          for (signed char dy = -5; dy < 5; dy++)
          {
            padded_image[x + dx][y + dy] = 0;
          }
        }
        // y += 6; // Alle celler bliver indfanget af detectoren, når vi hopper 6 pixels frem, så vi ikke tjekker samme område flere gange end nødvendigt.
      }
    }
    // x += 6;
  }
  for (int x = 6; x < BMP_WIDTH; x++)
  {
    for (int y = 6; y < BMP_HEIGHT; y++)
    {
      removed_cells_image[x][y] = padded_image[x][y];
    }
  }
}

void erode_and_detect_loop(unsigned char black_white_image[BMP_WIDTH][BMP_HEIGHT])
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

unsigned char input_image[BMP_WIDTH][BMP_HEIGHT][BMP_CHANNELS];
unsigned char output_image[BMP_WIDTH][BMP_HEIGHT][BMP_CHANNELS];
unsigned char grey_image[BMP_WIDTH][BMP_HEIGHT];
unsigned char black_white_image[BMP_WIDTH][BMP_HEIGHT];
unsigned char removed_cells_image[BMP_WIDTH][BMP_HEIGHT];

int main(int argc, char **argv)
{
  eroded_image = (unsigned char *) malloc(BMP_HEIGHT * BMP_WIDTH);
  printf("Eroded Image allokeret %zu bytes at address %p\n", BMP_HEIGHT * BMP_WIDTH, (void *) eroded_image);
  //Af en eller anden grund kan der ikke køre mere end én timer ad gangen. Så afkommentere den funktion man nu gerne vil teste.
  //start = clock();

  if (argc != 3)
  {
    fprintf(stderr, "Wrong main arguments. Use: %s <output file path> <output file path>\n", argv[0]);
    exit(1);
  }
  //TIMER_start();
  read_bitmap(argv[1], input_image);
  //TIMER_stop("Indlæsning af bitmap ");

  //TIMER_start();
  grey_scale(input_image, grey_image);
  //TIMER_stop("Grey Scale ");

  //TIMER_start();
  binary_threshold(grey_image, black_white_image);
  //TIMER_stop("Binary Threshold ");

  TIMER_start();
  erode_and_detect_loop(black_white_image);
  TIMER_stop("Erosion og detection ");

  //convert_2d_to_3d(removed_cells_image, output_image);

  //TIMER_start();
  insert_marks_at_cell_locations(input_image);
  //TIMER_stop("Indsætning af markering ");

  write_bitmap(input_image, argv[2]);



  // printf("Done!\n");
  printf("På billedet var antallet af celler lig med: %d\n", amount_of_cells);
  //end = clock();
  //cpu_time_used = end - start;
  //printf("Total time: %f sec\n", cpu_time_used/CLOCKS_PER_SEC);
  free(eroded_image);
  return 0;
}

