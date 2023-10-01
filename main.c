#include <stdlib.h>
#include <stdio.h>
#include "cbmp.h"
#include <string.h>
#include <unistd.h>
#include <fftw3.h> //Bibliotek til at lave fourier transformationer
#include <math.h>

unsigned char black_white_image[BMP_WIDTH][BMP_HEIGHT];
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
  double re;
  double im;
} ComplexCoordinate;

typedef struct
{
  Coordinate points[10000];
  int pixelCount;
  Coordinate boundary[10000];
  int boundaryCount;
  ComplexCoordinate smoothedBoundary[10000];
} Cluster;

Coordinate coordinates[1000];
Cluster clusters[300];
int clusterCount = 0;
fftw_complex output[10000];
fftw_complex ift_result[10000];
double slopes[10000];

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

void transpose(double **A, double **A_T, int rows, int cols)
{
  for (int i = 0; i < rows; i++)
  {
    for (int j = 0; j < cols; j++)
    {
      A_T[j][i] = A[i][j];
    }
  }
}

int invert(double **A, double **A_inv, int n)
{
  for (int i = 0; i < n; i++)
  {
    for (int j = 0; j < n; j++)
    {
      A_inv[i][j] = (i == j) ? 1.0 : 0.0;
    }
  }
  for (int i = 0; i < n; i++)
  {
    double max_val = fabs(A[i][i]);
    int pivot_row = i;
    for (int row = i + 1; row < n; row++)
    {
      if (fabs(A[row][i]) > max_val)
      {
        max_val = fabs(A[row][i]);
        pivot_row = row;
      }
    }
    if (i != pivot_row)
    {
      for (int j = 0; j < n; j++)
      {
        double tmp = A[i][j];
        A[i][j] = A[pivot_row][j];
        A[pivot_row][j] = tmp;

        tmp = A_inv[i][j];
        A_inv[i][j] = A_inv[pivot_row][j];
        A_inv[pivot_row][j] = tmp;
      }
    }
    double factor = A[i][i];
    if (fabs(factor) < 1e-8)
    {
      return -1;
    }
    for (int j = 0; j < n; j++)
    {
      A[i][j] /= factor;
      A_inv[i][j] /= factor;
    }
    for (int row = 0; row < n; row++)
    {
      if (row == i)
        continue;
      double factor = A[row][i];
      for (int j = 0; j < n; j++)
      {
        A[row][j] -= factor * A[i][j];
        A_inv[row][j] -= factor * A_inv[i][j];
      }
    }
  }
  return 0;
}

void multiply_matrices(double **A, double **B, double **C, int rowsA, int colsA, int colsB)
{
  for (int i = 0; i < rowsA; i++)
  {
    for (int j = 0; j < colsB; j++)
    {
      C[i][j] = 0.0;
      for (int k = 0; k < colsA; k++)
      {
        C[i][j] += A[i][k] * B[k][j];
      }
    }
  }
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
  printf("clusters start\n");
  Coordinate Area[10000];
  int pixelCount = 0;
  unsigned char visited[BMP_WIDTH][BMP_HEIGHT] = {{0}};
  unsigned char clusterFound;

  for (int x = 0; x < BMP_WIDTH; x++)
  {
    clusterFound = 0;
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
                  Area[pixelCount].x = currentX;
                  Area[pixelCount].y = currentY;
                  pixelCount++;
                  // printf("%d cluster pixels fundet\n", pixelCount);
                  enqueue(currentX, currentY);
                  visited[currentX][currentY] = 1;
                }
              }
            }
          }
          clusterFound = 1;
        }

        // Her er alle celler i cluster opdaget
        if (isEmpty() && clusterFound == 1)
        {
          // printf("Cluster registrering begyndt\n");
          //  Så her gemmer vi clusteret, alle dets coordinates og antallet af pixels i clusteret
          if (pixelCount >= 499)
          {
            for (int point = 0; point < pixelCount; point++)
            {
              clusters[clusterCount].points[point].x = Area[point].x;
              clusters[clusterCount].points[point].y = Area[point].y;
            }
            clusters[clusterCount].pixelCount = pixelCount;
            // printf("Pixel count = %d\n", pixelCount);
            // printf("Cluster %d bestaar af %d pixels\n", clusterCount, clusters[clusterCount].pixelCount);
            clusterCount++;
            // printf("%d Clustere registreret\n", clusterCount);
          }

          // Count og Area wipes
          for (int i = 0; i < pixelCount; i++)
          {
            Area[i].x = 0;
            Area[i].y = 0;
          }
          pixelCount = 0;
        }
      }
    }
  }
  printf("clusters end\n");
}

Coordinate find_cluster_center(Cluster cluster)
{
  int sumX = 0;
  int sumY = 0;
  for (int i = 0; i < cluster.pixelCount; i++)
  {
    sumX += cluster.points[i].x;
    sumY += cluster.points[i].y;
  }
  Coordinate center;
  center.x = sumX / cluster.pixelCount;
  center.y = sumY / cluster.pixelCount;
  return center;
}

Cluster find_cluster_boundary(Cluster cluster, unsigned char black_white_image[BMP_WIDTH][BMP_HEIGHT])
{
  int currentX = 0;
  int currentY = 0;
  int boundaryIndex = 0;
  unsigned char hasBlackNeighbor;
  for (int i = 0; i < cluster.pixelCount; i++)
  {
    hasBlackNeighbor = 0;
    currentX = cluster.points[i].x;
    currentY = cluster.points[i].y;
    for (int dx = -1; dx <= 1; dx++)
    {
      for (int dy = -1; dy <= 1; dy++)
      {
        if (currentX + dx < BMP_WIDTH && currentX + dx >= 0 && currentY + dy < BMP_HEIGHT && currentY + dy >= 0)
        {
          if (black_white_image[currentX + dx][currentY + dy] == 0)
          {
            hasBlackNeighbor = 1;
          }
        }
      }
    }
    if (hasBlackNeighbor)
    {
      Coordinate boundaryPixel;
      boundaryPixel.x = currentX;
      boundaryPixel.y = currentY;
      cluster.boundary[boundaryIndex] = boundaryPixel;
      boundaryIndex++;
      cluster.boundaryCount++;
    }
  }
  return cluster;
}

void boundary_to_freq_domain(Cluster *cluster)
{
  fftw_complex complex[10000];
  for (int i = 0; i < cluster->boundaryCount; i++)
  {
    complex[i][0] = cluster->boundary[i].x;
    complex[i][1] = cluster->boundary[i].y;
  }

  fftw_plan plan = fftw_plan_dft_1d(cluster->boundaryCount, complex, output, FFTW_FORWARD, FFTW_ESTIMATE);
  fftw_execute(plan);
  fftw_destroy_plan(plan);
}

void freq_to_spatial_domain(fftw_complex *output, int boundaryCount, Cluster *cluster)
{
  fftw_plan plan = fftw_plan_dft_1d(boundaryCount, output, ift_result, FFTW_BACKWARD, FFTW_ESTIMATE);
  fftw_execute(plan);
  fftw_destroy_plan(plan);

  for (int i = 0; i < boundaryCount; i++)
  {
    ift_result[i][0] /= boundaryCount;
    ift_result[i][1] /= boundaryCount;
  }
  for (int j = 0; j < boundaryCount; j++)
  {
    cluster->smoothedBoundary[j].re = ift_result[j][0];
    cluster->smoothedBoundary[j].im = ift_result[j][1];
  }
}

void store_slopes(double slopes[], double x[1][1], int index) {
    slopes[index] = x[0][0]; 
}

void calculate_slopes(Cluster *cluster)
{
  int N = 3; 
  int L = cluster->boundaryCount;
  double *slopes = malloc((L - 2 * N) * sizeof(double));

  double **A = malloc((2 * N + 1) * sizeof(double *));
  double **Y = malloc((2 * N + 1) * sizeof(double *));
  double **A_T = malloc(2 * sizeof(double *));
  double **A_T_A = malloc(2 * sizeof(double *));
  double **A_inv = malloc(2 * sizeof(double *));
  double **A_T_Y = malloc(2 * sizeof(double *));
  double **x = malloc(2 * sizeof(double *));

  for (int i = 0; i < 2 * N + 1; i++) {
    A[i] = malloc(2 * sizeof(double));
    Y[i] = malloc(sizeof(double));
  }

  for (int i = 0; i < 2; i++) {
    A_T[i] = malloc((2 * N + 1) * sizeof(double));
    A_T_A[i] = malloc(2 * sizeof(double));
    A_inv[i] = malloc(2 * sizeof(double));
    A_T_Y[i] = malloc((2 * N + 1) * sizeof(double));
    x[i] = malloc(sizeof(double));
  }

  // Main logic
  for (int j = N; j < L - N; j++) {
    for (int i = -N; i <= N; i++) {
      A[i + N][0] = j + i;
      A[i + N][1] = 1;
      Y[i + N][0] = sqrt(pow(cluster->smoothedBoundary[j + i].re, 2) + pow(cluster->smoothedBoundary[j + i].im, 2));
    }

    transpose(A, A_T, 2 * N + 1, 2);
    multiply_matrices(A_T, A, A_T_A, 2, 2 * N + 1, 2);
    invert(A_T_A, A_inv, 2);
    multiply_matrices(A_T, Y, A_T_Y, 2, 2 * N + 1, 1);
    multiply_matrices(A_inv, A_T_Y, x, 2, 2, 1);
    store_slopes(slopes, x, j - N);

    printf("Coordinates in x are %lf, %lf \n", x[0][0], x[1][0]);

    double threshold = 0.5;
    find_bottlenecks(slopes, L - 2 * N, threshold);

    // Free allocated memory
    free(slopes);
  }

  // Deallocate memory
  for (int i = 0; i < 2 * N + 1; i++) {
    free(A[i]);
    free(Y[i]);
  }

  for (int i = 0; i < 2; i++) {
    free(A_T[i]);
    free(A_T_A[i]);
    free(A_inv[i]);
    free(A_T_Y[i]);
    free(x[i]);
  }

  free(A);
  free(Y);
  free(A_T);
  free(A_T_A);
  free(A_inv);
  free(A_T_Y);
  free(x);
}

void find_bottlenecks(double slopes[], int size, double threshold) {
    for (int i = 1; i < size; i++) {
        if (fabs(slopes[i] - slopes[i-1]) > threshold) {
            printf("Bottleneck point at index %d\n", i);
        }
    }
}

void print_boundaries_and_smoothed(Cluster cluster)
{
  printf("Original Boundary Points:\n");
  for (int i = 0; i < cluster.boundaryCount; i++)
  {
    printf("(%d, %d) ", cluster.boundary[i].x, cluster.boundary[i].y);
  }
  printf("\n");

  printf("Smoothed Boundary Points via FFTW:\n");
  for (int i = 0; i < cluster.boundaryCount; i++)
  {
    printf("(%lf, %lf) ", cluster.smoothedBoundary[i].re, cluster.smoothedBoundary[i].im);
  }
  printf("\n");
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
  printf("started painting clusters\n");
  int currentX;
  int currentY;

  // printf("cluster painting begyndt\n");
  // printf("antallet af clusters er %d \n", clusterCount);

  for (int currentCluster = 0; currentCluster < clusterCount; currentCluster++)
  {
    // printf("Antallet af celler i cluster %d er %d\n", currentCluster, clusters[currentCluster].pixelCount);
    for (int currentCoordinate = 0; currentCoordinate < clusters[currentCluster].pixelCount; currentCoordinate++)
    {
      currentX = clusters[currentCluster].points[currentCoordinate].x;
      currentY = clusters[currentCluster].points[currentCoordinate].y;

      input_image[currentX][currentY][0] = 0;
      input_image[currentX][currentY][1] = 255;
      input_image[currentX][currentY][2] = 0;
    }
  }
  printf("finished painting clusters\n");
}

void paint_centers_blue(unsigned char input_image[BMP_WIDTH][BMP_HEIGHT][BMP_CHANNELS])
{
  printf("started painting centers\n");
  Coordinate center;
  int centerX;
  int centerY;

  for (int currentCluster = 0; currentCluster < clusterCount; currentCluster++)
  {
    center = find_cluster_center(clusters[currentCluster]);
    centerX = center.x;
    centerY = center.y;

    for (int dx = -3; dx <= 3; dx++)
    {
      for (int dy = -3; dy <= 3; dy++)
      {
        int paintX = centerX + dx;
        int paintY = centerY + dy;

        // Check boundary conditions
        if (paintX >= 0 && paintX < BMP_WIDTH && paintY >= 0 && paintY < BMP_HEIGHT)
        {
          input_image[paintX][paintY][0] = 0;
          input_image[paintX][paintY][1] = 0;
          input_image[paintX][paintY][2] = 255;
        }
      }
    }
  }
  printf("finished painting centers\n");
}

void paint_boundaries_red(unsigned char input_image[BMP_WIDTH][BMP_HEIGHT][BMP_CHANNELS], unsigned char black_white_image[BMP_WIDTH][BMP_HEIGHT])
{
  printf("started painting boundaries\n");
  int currentX;
  int currentY;
  Cluster updatedCluster;

  for (int currentCluster = 0; currentCluster < clusterCount; currentCluster++)
  {
    updatedCluster = find_cluster_boundary(clusters[currentCluster], black_white_image);

    for (int i = 0; i < updatedCluster.boundaryCount; i++)
    {
      currentX = updatedCluster.boundary[i].x;
      currentY = updatedCluster.boundary[i].y;

      // Check boundary conditions
      if (currentX >= 0 && currentX < BMP_WIDTH && currentY >= 0 && currentY < BMP_HEIGHT)
      {
        input_image[currentX][currentY][0] = 255;
        input_image[currentX][currentY][1] = 0;
        input_image[currentX][currentY][2] = 0;
      }
    }
  }
  printf("finished painting boundaries\n");
}

void paint_bottlenecks(unsigned char input_image[BMP_WIDTH][BMP_HEIGHT][BMP_CHANNELS], Cluster currentCluster, double slopes[], double threshold)
{
    int N = 3; // The same N value used in calculate_slopes
    int L = currentCluster.boundaryCount;
    int currentX;
    int currentY;

    for (int i = 1; i < L - 2 * N; i++) 
    {
        if (fabs(slopes[i] - slopes[i - 1]) > threshold) 
        {
            currentX = currentCluster.boundary[i + N].x;
            currentY = currentCluster.boundary[i + N].y;

            if (currentX >= 0 && currentX < BMP_WIDTH && currentY >= 0 && currentY < BMP_HEIGHT) 
            {
                input_image[currentX][currentY][0] = 255;
                input_image[currentX][currentY][1] = 0;
                input_image[currentX][currentY][2] = 0;
            }
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
    // sleep(sleep_time);

    // detect cells and print
    detect_cells(eroded_image, removed_cells_image);
    convert_2d_to_3d(removed_cells_image, output_image);
    write_bitmap(output_image, "output_images/LiveProcess.bmp");
    // sleep(sleep_time);

    // insert marks and print
    insert_marks_at_cell_locations(input_image);
    write_bitmap(input_image, "output_images/LiveProcess.bmp");
    // sleep(sleep_time);

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
unsigned char eroded_image[BMP_WIDTH][BMP_HEIGHT];
unsigned char removed_cells_image[BMP_WIDTH][BMP_HEIGHT];
Cluster currentCluster;

int main(int argc, char **argv)
{
  printf("program started\n");
  if (argc != 3)
  {
    fprintf(stderr, "Wrong main arguments. Use: %s <output file path> <output file path>\n", argv[0]);
    exit(1);
  }

  read_bitmap(argv[1], input_image);

  grey_scale(input_image, grey_image);

  binary_threshold(grey_image, black_white_image);

  find_cell_clusters(black_white_image);

  paint_clusters_green(input_image);

  paint_centers_blue(input_image);

  paint_boundaries_red(input_image, black_white_image);

  write_bitmap(input_image, argv[2]);
  double threshold = 0.5;

  for (int i = 0; i < clusterCount; i++)
  {
    currentCluster = find_cluster_boundary(clusters[i], black_white_image);

    boundary_to_freq_domain(&currentCluster);

    freq_to_spatial_domain(output, currentCluster.boundaryCount, &currentCluster);

    calculate_slopes(&currentCluster);

    paint_bottlenecks(input_image, currentCluster, slopes, threshold);
  }
  

  // erode_and_detect_loop(black_white_image, argv[2]);

  printf("Paa billedet var antallet af celler lig med: %d\n", amount_of_cells);
  return 0;
}