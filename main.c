#include <stdlib.h>
#include <stdio.h>
#include "cbmp.h"
#include <string.h>

unsigned int amount_of_cells = 0;
unsigned char erosion_happened = 0;

typedef struct
{
  int x;
  int y;
} Point;

// unsigned int initial_size = 300;
// Point *points;
// unsigned int current_size;
// unsigned int num_points = 0;

void grey_scale(unsigned char rgb_image[BMP_WIDTH][BMP_HEIGHT][BMP_CHANNELS], unsigned char grey_scale_image[BMP_WIDTH][BMP_HEIGHT])
{
  for (int x = 0; x < BMP_WIDTH; x++)
  {
    for (int y = 0; y < BMP_HEIGHT; y++)
    {
      unsigned char red = rgb_image[x][y][0];
      unsigned char green = rgb_image[x][y][1];
      unsigned char blue = rgb_image[x][y][2];

      unsigned char grey = (red + green + blue) / 3;
      // binray threshhold kan laves her. så kan man slippe for division, og for at lagre den midlertidige resultat grey_scale_image i et array

      grey_scale_image[x][y] = grey;
    }
  }
}

void binary_threshold(unsigned char grey_scale_image[BMP_WIDTH][BMP_HEIGHT], unsigned char black_white_image[BMP_WIDTH][BMP_HEIGHT])
{
  for (int x = 0; x < BMP_WIDTH; x++)
  {
    for (int y = 0; y < BMP_HEIGHT; y++)
    {
      unsigned char threshold = 90;
      if (grey_scale_image[x][y] <= threshold)
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

void erode(unsigned char black_white_image[BMP_WIDTH][BMP_HEIGHT], unsigned char eroded_image[BMP_WIDTH][BMP_HEIGHT])
{
  unsigned char erosion_structure[3][3] = {{0, 1, 0}, {1, 1, 1}, {0, 1, 0}};
  unsigned char temp_image[BMP_WIDTH][BMP_HEIGHT];

  for (int x = 0; x < BMP_WIDTH; x++)
  {
    for (int y = 0; y < BMP_HEIGHT; y++)
    {
      temp_image[x][y] = black_white_image[x][y];
    }
  }

  erosion_happened = 0;
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
            shouldErode = 1;
            break;
          }
        }
        if (shouldErode)
          break;
      }
      if (shouldErode)
        temp_image[x][y] = 0;
      erosion_happened = 1;
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
  unsigned char padded_image[BMP_WIDTH + 12][BMP_HEIGHT + 12];
  memset(padded_image, 0, sizeof(padded_image)); // fylder padded_image med 0'er

  for (unsigned int x = 6; x < BMP_WIDTH; x++)
  {
    for (unsigned int y = 6; y < BMP_HEIGHT; y++)
    {
      padded_image[x][y] = eroded_image[x][y];
    }
  }

  for (unsigned int x = 5; x <= BMP_WIDTH - 5; x++)
  {
    for (unsigned int y = 5; y <= BMP_HEIGHT - 5; y++)
    {
      unsigned char white_in_inner_square = 0;
      unsigned char white_in_outer_square = 0;
      unsigned char cell_detected = 0;

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
            if (white_in_inner_square && !white_in_outer_square)
              cell_detected = 1;
          }
        }
      }
      // Tilfoejer 1 til maengden af celler, gemmer koordinater og farver den indre firkant sort, hvis en celle bliver fundet
      if (cell_detected)
      {
        amount_of_cells++;
        // if (num_points >= current_size)
        // {
        //   current_size *= 2;
        //   points = realloc(points, current_size * sizeof(Point));
        // }
        // points[num_points].x = x;
        // points[num_points].y = y;
        // num_points++;

        for (signed char dx = -5; dx < 5; dx++)
        {
          for (signed char dy = -5; dy < 5; dy++)
          {
            padded_image[x + dx][y + dy] = 0;
          }
        }
        y += 9; // Alle celler bliver indfanget af detectoren, når vi hopper 9 pixels frem. Jeg er ikke sikker på, hvorfor det lige præcis er 9 pixels.
      }
    }
    x += 9;
  }
  for (int x = 0; x < BMP_WIDTH; x++)
  {
    for (int y = 0; y < BMP_HEIGHT; y++)
    {
      removed_cells_image[x][y] = eroded_image[x][y];
    }
  }
}

void erode_and_detect_loop(unsigned char black_white_image[BMP_WIDTH][BMP_HEIGHT])
{
  unsigned char eroded_image[BMP_WIDTH][BMP_HEIGHT];
  unsigned char removed_cells_image[BMP_WIDTH][BMP_HEIGHT];

  erode(black_white_image, eroded_image);
  if (erosion_happened)
  {
    detect_cells(eroded_image, removed_cells_image);
    erode_and_detect_loop(removed_cells_image);
  }
  else if (!erosion_happened)
  {
    printf("Igen erosion skete. Alle celler er opdaget! Bestemmer placering af celler...\n");
  }
  {
  }
}

void insert_crosses_at_cell_locations(unsigned char input_image[BMP_WIDTH][BMP_HEIGHT][BMP_CHANNELS], Point *points, unsigned int num_points, unsigned char image_with_cell_locations[BMP_WIDTH][BMP_HEIGHT][BMP_CHANNELS])
{
  // Loop henover alle koordinater for cellerne, skriv algoritme som indsæter røde krydser ud fra hvert punkt, indsæt krydserne på input billede, print outputbillede som input billede + ryde krydser
  //  Copy input_image to image_with_cell_locations
  for (int x = 0; x < BMP_WIDTH; x++)
  {
    for (int y = 0; y < BMP_HEIGHT; y++)
    {
      for (int c = 0; c < BMP_CHANNELS; c++)
      {
        image_with_cell_locations[x][y][c] = input_image[x][y][c];
      }
    }
  }

  // Insert red crosses
  for (unsigned int i = 0; i < num_points; i++)
  {
    int center_x = points[i].x;
    int center_y = points[i].y;

    for (int dx = -2; dx <= 2; dx++)
    {
      for (int dy = -2; dy <= 2; dy++)
      {
        if (dx == 0 || dy == 0)
        {
          if (center_x + dx >= 0 && center_x + dx < BMP_WIDTH && center_y + dy >= 0 && center_y + dy < BMP_HEIGHT)
          {
            image_with_cell_locations[center_x + dx][center_y + dy][0] = 255;
            image_with_cell_locations[center_x + dx][center_y + dy][1] = 0;
            image_with_cell_locations[center_x + dx][center_y + dy][2] = 0;
          }
        }
      }
    }
  }
}

void convert_2d_to_3d(unsigned char image_2d[BMP_WIDTH][BMP_HEIGHT], unsigned char image_3d[BMP_WIDTH][BMP_HEIGHT][BMP_CHANNELS])
{
  for (int x = 0; x < BMP_WIDTH; x++)
  {
    for (int y = 0; y < BMP_HEIGHT; y++)
    {
      image_3d[x][y][0] = image_2d[x][y];
      image_3d[x][y][1] = image_2d[x][y];
      image_3d[x][y][2] = image_2d[x][y];
    }
  }
}

unsigned char input_image[BMP_WIDTH][BMP_HEIGHT][BMP_CHANNELS];
unsigned char output_image[BMP_WIDTH][BMP_HEIGHT][BMP_CHANNELS];
unsigned char grey_image[BMP_WIDTH][BMP_HEIGHT];
unsigned char black_white_image[BMP_WIDTH][BMP_HEIGHT];
unsigned char image_with_cell_locations[BMP_WIDTH][BMP_HEIGHT][BMP_CHANNELS];

int main(int argc, char **argv)
{
  if (argc != 3)
  {
    fprintf(stderr, "Wrong main arguments. Use: %s <output file path> <output file path>\n", argv[0]);
    exit(1);
  }
  // Array med koordinater (sizes er declared i starten af filen)
  // current_size = initial_size;
  // points = malloc(initial_size * sizeof(Point));

  read_bitmap(argv[1], input_image);

  grey_scale(input_image, grey_image);

  binary_threshold(grey_image, black_white_image);

  erode_and_detect_loop(black_white_image);

  //insert_crosses_at_cell_locations(input_image, points, num_points, image_with_cell_locations);

  //convert_2d_to_3d(image_with_cell_locations, output_image);

  //write_bitmap(image_with_cell_locations, argv[2]);

  printf("Paa billedet var antallet af celler lig med: %d\n", amount_of_cells);
  //free(points); // Gør alle de allokerede pladser i points array'et frie igen, hvis de ikke bliver brugt
  return 0;
}