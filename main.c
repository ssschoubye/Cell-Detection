#include <stdlib.h>
#include <stdio.h>
#include "cbmp.h"
#include <string.h>
#include <unistd.h>

unsigned char eroded_image[BMP_WIDTH][BMP_HEIGHT];
unsigned char removed_cells_image[BMP_WIDTH][BMP_HEIGHT];
unsigned char input_image[BMP_WIDTH][BMP_HEIGHT][BMP_CHANNELS];
unsigned char output_image[BMP_WIDTH][BMP_HEIGHT][BMP_CHANNELS];

unsigned int amount_of_cells = 0;
unsigned int erosion_happened;

typedef struct
{
  unsigned int x;
  unsigned int y;
} Coordinate;

typedef struct
{
  Coordinate points[1000];
  int count;
} Cluster;

Coordinate coordinates[1000];
Cluster clusters[100];
int clusterCount = 0;

Coordinate queue[BMP_WIDTH * BMP_HEIGHT];
int front = 0, rear = -1;

void enqueue(int x, int y)
{
  if (rear < BMP_WIDTH * BMP_HEIGHT - 1)
  {
    rear++;
    queue[rear].x = x;
    queue[rear].y = y;
  }
  else
  {
    printf("Overflow error");
  }
}

Coordinate dequeue()
{
  return queue[front++];
}

int isEmpty()
{
  return front > rear;
}

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

void find_cell_clusters(unsigned char black_white_image[BMP_WIDTH][BMP_HEIGHT])
{
  printf("clusters start");
  Coordinate Area[1000];
  int count = 0;
  int visited[BMP_WIDTH][BMP_HEIGHT] = {{0}};

  for (int x = 0; x < BMP_WIDTH; x++)
  {
    for (int y = 0; y < BMP_HEIGHT; y++)
    {
      if (black_white_image[x][y] == 255 && visited[x][y] == 0)
      {
        // Et punkt bliver fundet, vi tilføjer det til queue og starter vores while loop som finder alle hvide naboer iterativt
        enqueue(x, y);
        while (!isEmpty())
        {
          // Vi dequeuer de fundne punkt, og bruger det som centrum i vores search structure
          Coordinate p = dequeue();

          for (int dx = -1; dx <= 1; dx++)
          {
            for (int dy = -1; dy <= 1; dy++)
            {
              if (p.x + dx < BMP_WIDTH && p.x + dx >= 0 && p.y + dy < BMP_HEIGHT && p.y + dy >= 0)
              {
                int currentX = p.x + dx;
                int currentY = p.y + dy;
                if (black_white_image[currentX][currentY] == 255 && visited[currentX][currentY] == 0)
                {
                  Area[count].x = currentX;
                  Area[count].y = currentY;
                  count++;
                  enqueue(currentX, currentY);
                  visited[currentX][currentY] = 1;
                }
              }
            }
          }
        }

        // TODO fjern duplicates fra area. Vi har valgt ikke at gøre dette, fordi celler ikke bliver tilføjet til area, hvis de er visited.
        // Og det er igen bedre at tælle noget som et cluster, selvom det ikke er, end omvendt.

        // Her er alle celler i cluster opdaget
        if (isEmpty())
        {
          // Så her gemmer vi clusteret, alle dets coordinates og antallet af pixels i clusteret
          if (count >= 400)
          {
            // Her sætter vi tersklen til 400. Vi har talt, og nogen enkelte celler er lidt større end 400, og nogle clusters er lidt mindre end 400.
            // Men det virker som en fin value indtil videre, da det er bedre at betragte noget som et cluster, selvom det ikke er det,
            // end at ikke behandle noget som et cluster, når det er det
            for (int point = 0; point < count; point++)
            {
              clusters[clusterCount].points[point].x = Area[point].x;
              clusters[clusterCount].points[point].y = Area[point].y;
            }
            clusters[clusterCount].count = count;
            clusterCount++;
          }

          // Count og Area wipes
          for (int i = 0; i < count; i++)
          {
              Area[i].x = 0;
              Area[i].y = 0;
          }
          count = 0;
        }
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
          // if (dx == -6 || dx == 6 || dy == -6 || dy == 6)
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

void paint_clusters_green(unsigned char input_image[BMP_WIDTH][BMP_HEIGHT][BMP_CHANNELS])
{
  int currentX;
  int currentY;

  for (int currentCluster = 0; currentCluster < clusterCount; currentCluster++)
  {
    for (int currentCoordinate = 0; currentCoordinate < clusters[clusterCount].count; currentCoordinate++)
    {
      currentX = clusters[clusterCount].points[currentCoordinate].x;
      currentY = clusters[clusterCount].points[currentCoordinate].y;

      input_image[currentX][currentY][1] = 255;
    }
  }
}


void erode_and_detect_loop(unsigned char black_white_image[BMP_WIDTH][BMP_HEIGHT], char *output_file_path)
{
  int sleep_time = 1;
  // erode and print
  erode(black_white_image, eroded_image);

  if (erosion_happened)
  {
    // print the eroded image
    convert_2d_to_3d(eroded_image, output_image);
    write_bitmap(output_image, "output_images/LiveProcess.bmp");
    sleep(sleep_time);

    // detect cells and print
    detect_cells(eroded_image, removed_cells_image);
    convert_2d_to_3d(removed_cells_image, output_image);
    write_bitmap(output_image, "output_images/LiveProcess.bmp");
    sleep(sleep_time);

    // insert marks and print
    insert_marks_at_cell_locations(input_image);
    write_bitmap(input_image, "output_images/LiveProcess.bmp");
    sleep(sleep_time);

    // Recurse
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

  if (argc != 3)
  {
    fprintf(stderr, "Wrong main arguments. Use: %s <output file path> <output file path>\n", argv[0]);
    exit(1);
  }

  read_bitmap(argv[1], input_image);

  grey_scale(input_image, grey_image);

  binary_threshold(grey_image, black_white_image);

  printf("foer clusters");
  find_cell_clusters(black_white_image);

  paint_clusters_green(input_image);

  write_bitmap(input_image, argv[2]);

  // erode_and_detect_loop(black_white_image, argv[2]);


  printf("Paa billedet var antallet af celler lig med: %d\n", amount_of_cells);
  return 0;
}