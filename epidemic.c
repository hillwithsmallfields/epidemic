#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <getopt.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>

/* We can build a tree of who was infected by who; but we don't yet
   have any way of reading this data out, so I've turned it off for
   now.  In the real world, someone might be infected by more than one
   person, of course, so this may not be that useful. */
// #define TRACING 1

/* Output PNG files to show the spread */
// #define PRODUCE_IMAGES 1

#ifdef PRODUCE_IMAGES
#include <png.h>
#endif

/* Definitions for the bitfield sizes which also have other things
   based on them, so they can be changed easily and be tracked by
   their dependents: */
#define SPREADER_GRADE_BITS 2

/* A very compact representation of a person, so we can do millions of
   them on a fairly ordinary machine. */
typedef struct person_t {
  unsigned int days_in_state  : 10;
  unsigned int state          : 4;
  unsigned int spreader_grade : SPREADER_GRADE_BITS;
  unsigned int age            : 7;
#ifdef TRACING
  unsigned int infected_by;
#endif
} person_t;

/* Some counts that we will maintain as people move between states */
typedef struct counts_t {
    unsigned int susceptible;
    unsigned int incubating;
    unsigned int asymptomatic;
    unsigned int carrying;
    unsigned int ill;
    unsigned int recovered;
    unsigned int vaccinated;
    unsigned int died;
} counts_t;

typedef struct population_grid_t {
    unsigned int population_size;
    unsigned int grid_width;
    unsigned int grid_height;
    person_t *population;
} population_grid_t;

/* The possible values for the 'state' field: */
typedef enum state {
    NOBODY = 0,
    SUSCEPTIBLE,
    INCUBATING,
    ASYMPTOMATIC,
    CARRYING,
    ILL,
    RECOVERED,
    VACCINATED,
    DIED
} state_t;

/* Treating the population array as a two-dimensional grid, for
   purposes of who is near who (and so who can be infected by who): */
#define Neighbour(_pop_, _base_, _dx_, _dy_) (((_base_) + (_dx_) + ((_dy_) * (_pop_).grid_width)) % (_pop_).population_size)

#define Beyond(_bits_) (1 << _bits_)

/* TODO: https://wellcomeopenresearch.org/articles/5-67 says: "80% of secondary transmissions may have been caused by a small fraction of infectious individuals (~10%)" */
#define N_SPREADER_GRADES Beyond(SPREADER_GRADE_BITS)

/* We need to know the age distribution.  Here is one I extracted from
   a spreadsheet on a UK government website.  The risk of death is
   from the Chinese values on the chart at
   https://ourworldindata.org/mortality-risk-covid#case-fatality-rate-of-covid-19-by-age */
static double default_age_data[] = {
  // age, number in population, risk of catching, relative risk of death
  0, 712544, 1, 0,
  1, 731552, 1, 0,
  2, 719903, 1, 0,
  3, 728775, 1, 0,
  4, 749716, 1, 0,
  5, 755165, 1, 0,
  6, 781296, 1, 0,
  7, 790079, 1, 0,
  8, 776092, 1, 0,
  9, 770442, 1, 0,
  10, 780027, 1, 0.002,
  11, 762286, 1, 0.002,
  12, 752187, 1, 0.002,
  13, 755586, 1, 0.002,
  14, 725424, 1, 0.002,
  15, 721461, 1, 0.002,
  16, 722828, 1, 0.002,
  17, 736831, 1, 0.002,
  18, 729094, 1, 0.002,
  19, 699477, 1, 0.002,
  20, 665903, 1, 0.002,
  21, 659726, 1, 0.002,
  22, 689704, 1, 0.002,
  23, 713489, 1, 0.002,
  24, 740562, 1, 0.002,
  25, 793528, 1, 0.002,
  26, 842028, 1, 0.002,
  27, 874752, 1, 0.002,
  28, 869510, 1, 0.002,
  29, 898289, 1, 0.002,
  30, 909243, 1, 0.002,
  31, 933090, 1, 0.002,
  32, 937354, 1, 0.002,
  33, 948525, 1, 0.002,
  34, 941315, 1, 0.002,
  35, 924873, 1, 0.002,
  36, 902650, 1, 0.002,
  37, 883249, 1, 0.002,
  38, 849669, 1, 0.002,
  39, 833083, 1, 0.002,
  40, 820064, 1, 0.004,
  41, 794797, 1, 0.004,
  42, 770328, 1, 0.004,
  43, 749944, 1, 0.004,
  44, 756457, 1, 0.004,
  45, 749170, 1, 0.004,
  46, 738565, 1, 0.004,
  47, 747581, 1, 0.004,
  48, 768527, 1, 0.004,
  49, 793618, 1, 0.004,
  50, 848926, 1, 0.013,
  51, 905094, 1, 0.013,
  52, 728019, 1, 0.013,
  53, 713446, 1, 0.013,
  54, 706140, 1, 0.013,
  55, 676500, 1, 0.013,
  56, 614232, 1, 0.013,
  57, 574955, 1, 0.013,
  58, 599775, 1, 0.013,
  59, 602612, 1, 0.013,
  60, 596737, 1, 0.036,
  61, 583322, 1, 0.036,
  62, 568631, 1, 0.036,
  63, 552998, 1, 0.036,
  64, 533180, 1, 0.036,
  65, 529311, 1, 0.036,
  66, 534019, 1, 0.036,
  67, 535086, 1, 0.036,
  68, 523636, 1, 0.036,
  69, 505858, 1, 0.036,
  70, 487741, 1, 0.08,
  71, 483403, 1, 0.08,
  72, 476279, 1, 0.08,
  73, 458484, 1, 0.08,
  74, 445259, 1, 0.08,
  75, 431429, 1, 0.08,
  76, 433618, 1, 0.08,
  77, 433515, 1, 0.08,
  78, 419602, 1, 0.08,
  79, 284208, 1, 0.08,
  80, 244002, 1, 0.148,
  81, 252357, 1, 0.148,
  82, 248742, 1, 0.148,
  83, 242710, 1, 0.148,
  84, 221012, 1, 0.148,
  85, 1079747, 1, 0.148
};

#define Age(_i_)                (age_data[(_i_)*4 + 0])
#define Age_Number(_i_)         (age_data[(_i_)*4 + 1])
#define Age_Susceptibility(_i_) (age_data[(_i_)*4 + 2])
#define Age_Mortality(_i_)      (age_data[(_i_)*4 + 3])

static double default_spreader_data[] = {
// grade, proportion, R,   distance
   0,     .1,         0.1, 0,
   1,     .4,         0.9, 1,
   2,     .4,         1.5, 3,
   3,     .1,         12,  4
};

#define Spreader_Grade(_i_)      (spreader_data[(_i_)*4 + 0])
#define Spreader_Proportion(_i_) (spreader_data[(_i_)*4 + 1])
#define Spreader_R(_i_)          (spreader_data[(_i_)*4 + 2])
#define Spreader_Radius(_i_)     (spreader_data[(_i_)*4 + 3])

static const char *short_options = "a:c:g:hi:I:o:p:P:R:s:v";

struct option long_options_data[] = {
  {"age", required_argument, 0, 'a'},
  {"cycles", required_argument, 0, 'c'},
  {"grades", required_argument, 0, 'g'},
  {"help", no_argument, 0, 'h'},
  {"population", required_argument, 0, 'p'},
  {"pictures", required_argument, 0, 'P'},
  {"reproduction", required_argument, 0, 'R'},
  {"starting", required_argument, 0, 's'},
  {"infectious", required_argument, 0, 'i'},
  {"interventions", required_argument, 0, 'I'},
  {"output", required_argument, 0, 'o'},
  {"verbose", no_argument, 0, 'v'},
  {0, 0, 0, 0}
};

struct option *long_options = long_options_data;

/* We make up a couple of temporary arrays for looking up scaled
   random numbers and returning values in a given distribution (for
   spreader grades, and ages).  This is the number of points in these
   arrays. */
#define DISTRIBUTION_POINTS 4096

/* Read a CSV file into an array of doubles.

   If the first character of the file is a letter, the first row is skipped as being a header.

   Cells that don't contain a number (or contain other text) are
   copied from the same cell in the row above; with the argument
   zero_means_repeat, this also happens for cells containing zero.
*/
static unsigned int read_table(char *filename, double **data, unsigned int *rows, unsigned int *columns, int zero_means_repeat) {
    struct stat filestats;
    if (stat(filename, &filestats) != 0) {
        fprintf(stderr, "Could not examine table file \"%s\"\n", filename);
        return 0;
    }
    off_t filesize = filestats.st_size;
    char *filedata = (char*)malloc(filesize+1);
    int filedesc = open(filename, O_RDONLY);
    if (filedesc == -1) {
        fprintf(stderr, "Could not open table file \"%s\"\n", filename);
        return 0;
    }
    if (read(filedesc, filedata, filesize) != filesize) {
        fprintf(stderr, "Could not read all of table file \"%s\"\n", filename);
        return 0;
    }
    unsigned int row_counter = 0;
    unsigned int col_counter = 0;
    unsigned int widest = 1;

    /* use this to skip a header line, while still being able to
       free() the block: */
    char *filedata_start = filedata;
    char *filedata_end = &filedata[filesize];

    if (isalpha(*filedata_start)) {
        while (filedata_start < filedata_end && *filedata_start != '\n') {
            filedata_start++;
        }
        filedata_start++;
    }

    for (char *p = filedata_start; p < filedata_end; p++) {
        switch (*p) {
        case '\n':
            col_counter++;
            if (col_counter > widest) {
                widest = col_counter;
            }
            col_counter = 0;
            row_counter++;
            break;
        case ',':
            col_counter++;
            break;
        default:
            break;
        }
    }

    unsigned int row = 0;
    unsigned int column = 0;

    *filedata_end = '\0';
    double *parsed_data = (double*)malloc(row_counter*widest*sizeof(double));
    char *p = filedata_start;
    while (p < filedata_end) {
        char *cell_start = p;
        int has_number = 0;
        int has_non_number = 0;
        int eol = 0;
        char *q = p;
        char c = *q;

        while (c != '\0') {
            if (c == ',') {
                *q = '\0';
                break;
            } else if (c == '\n') {
                *q = '\0';
                eol = 1;
                break;
            }
            if (isalpha(c)) {
                has_non_number = 1;
            } else if (isdigit(c) || c=='.') {
                has_number = 1;
            }
            c = *++q;
        }
        p = q+1;
        if (has_number && !has_non_number) {
            double f = atof(cell_start);
            parsed_data[row*widest + column] = f;
            if (zero_means_repeat && f == 0 && row >= 1) {
                parsed_data[row*widest + column] = parsed_data[(row-1)*widest + column];
            }
        } else if (row >= 1) {
            parsed_data[row*widest + column] = parsed_data[(row-1)*widest + column];
        }
        if (eol) {
            row++;
            column = 0;
        } else {
            column++;
        }
    }

    free(filedata);
    *data = parsed_data;
    *rows = row_counter;
    *columns = widest;
    return filesize;
}

static void
print_usage() {
  printf("Usage:\n  growth\n");
}

static double *spreader_data = NULL;
static unsigned int infectious_days = 10;

static void infect(unsigned int who, population_grid_t *population, counts_t *counts) {
    double personal_r = Spreader_R(population->population[who].spreader_grade);
    while (personal_r / infectious_days >= 0.0) {
        if (drand48() < personal_r / infectious_days) {
            /* we're going to infect someone, pick someone in our travel range */
            unsigned int personal_travel_limit = (unsigned int)Spreader_Radius(population->population[who].spreader_grade);
            int delta_x = (int)((drand48() * 2.0 - 1.0) * personal_travel_limit);
            int delta_y = (int)((drand48() * 2.0 - 1.0) * personal_travel_limit);
            if (delta_x != 0 || delta_y != 0) {
                person_t *neighbour = &(population->population)[Neighbour(*population, who, delta_x, delta_y)];
                if (neighbour->state == SUSCEPTIBLE) {
                    neighbour->state = INCUBATING;
                    neighbour->days_in_state = 0;
#ifdef TRACING
                    neighbour->infected_by = who;
#endif
                    counts->susceptible--;
                    counts->incubating++;
                }
            }
        }
        personal_r -= 1.0 / infectious_days;
    }
}

#define Intervention_Day(_i_)          (interventions_data[(_i_) * interventions_columns + 0])
#define Intervention_Vaccinations(_i_) (interventions_data[(_i_) * interventions_columns + 1])
#define Intervention_R(_i_, _j_)       (interventions_data[(_i_) * interventions_columns + 2 + (2*(_j_) + 0)])
#define Intervention_Radius(_i_, _j_)  (interventions_data[(_i_) * interventions_columns + 2 + (2*(_j_) + 1)])
#define Intervention_Grades()          ((interventions_columns - 2) / 2)

#ifdef PRODUCE_IMAGES

static unsigned int RGBs[8][3] = {
    {127,127,126}, // NOBODY       0
    {255,255,255}, // SUSCEPTIBLE  1
    {255,105,180}, // INCUBATING   2
    {127,127,127}, // ASYMPTOMATIC 3
    {255,165,0},   // CARRYING     4
    {255,0,0},     // ILL          5
    {0,255,0},     // RECOVERED    6
    {0,0,255},     // VACCINATED   7
    {0,0,0},       // DIED         8
};

/* based on http://www.labbookpages.co.uk/software/imgProc/files/libPNG/makePNG.c */

int writeImage(char* filename,
               population_grid_t *grid,
               char* title)
{
        int code = 0;
        FILE *fp = NULL;
        png_structp png_ptr = NULL;
        png_infop info_ptr = NULL;
        png_bytep row = NULL;
        
        // Open file for writing (binary mode)
        fp = fopen(filename, "wb");
        if (fp == NULL) {
                fprintf(stderr, "Could not open file %s for writing\n", filename);
                code = 1;
                goto finalise;
        }

        // Initialize write structure
        png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
        if (png_ptr == NULL) {
                fprintf(stderr, "Could not allocate write struct\n");
                code = 1;
                goto finalise;
        }

        // Initialize info structure
        info_ptr = png_create_info_struct(png_ptr);
        if (info_ptr == NULL) {
                fprintf(stderr, "Could not allocate info struct\n");
                code = 1;
                goto finalise;
        }

        // Setup Exception handling
        if (setjmp(png_jmpbuf(png_ptr))) {
                fprintf(stderr, "Error during png creation\n");
                code = 1;
                goto finalise;
        }

        png_init_io(png_ptr, fp);

        // Write header (8 bit colour depth)
        png_set_IHDR(png_ptr, info_ptr, grid->grid_width, grid->grid_height,
                        8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
                        PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

        // Set title
        if (title != NULL) {
                png_text title_text;
                title_text.compression = PNG_TEXT_COMPRESSION_NONE;
                title_text.key = "Title";
                title_text.text = title;
                png_set_text(png_ptr, info_ptr, &title_text, 1);
        }

        png_write_info(png_ptr, info_ptr);

        // Allocate memory for one row (3 bytes per pixel - RGB)
        row = (png_bytep) malloc(3 * grid->grid_width * sizeof(png_byte));

        // Write image data
        int x, y;
        for (y=0 ; y<grid->grid_height ; y++) {
                for (x=0 ; x<grid->grid_width ; x++) {
                    png_byte *ptr = &(row[x*3]);
                    unsigned int *colour = RGBs[grid->population[y*grid->grid_width + x].state % 8];
                    ptr[0] = colour[0];
                    ptr[1] = colour[1];
                    ptr[2] = colour[2];
                }
                png_write_row(png_ptr, row);
        }

        // End write
        png_write_end(png_ptr, NULL);

        finalise:
        if (fp != NULL) fclose(fp);
        if (info_ptr != NULL) png_free_data(png_ptr, info_ptr, PNG_FREE_ALL, -1);
        if (png_ptr != NULL) png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
        if (row != NULL) free(row);

        return code;
}
#endif

int main(int argc, char **argv) {
  int verbose = 0;
  int cycles = 365;
  unsigned int starting_cases = 1;
  double reproduction_rate = 3.0;
  unsigned int incubation_days = 2;
  unsigned int carrying_days = 5;
  unsigned int ill_days = 10;
  unsigned int asymptomatic_days = 20;

  char *spreader_grades_file = NULL;
  char *age_distribution_file = NULL;
  char *interventions_file = NULL;

  population_grid_t population = {1024 * 1024, 1024, 1024};
  counts_t counts = {0, 0, 0, 0, 0, 0, 0};
  counts_t previous_counts = {0, 0, 0, 0, 0, 0, 0};

  FILE *outstream = stdout;
#ifdef PRODUCE_IMAGES
  char *image_filename_format = "day-%03d.png";
  char *image_filename_buffer;
  char title_buffer[16];
#endif
  clock_t begin = clock();
  
  while (1) {
    int option_index = 0;
    char opt = getopt_long(argc, argv,
                           short_options, long_options,
                           &option_index);

    // fprintf(stderr, "Got opt %c\n", opt);

    if (opt == -1) {
      break;
    }
    switch (opt) {
    case 'a':
        age_distribution_file = optarg;
        break;
    case 'c':
        cycles = atoi(optarg);
        break;
    case 'h':
        print_usage();
        exit(0);
    case 'i':
        infectious_days = atoi(optarg);
        break;
    case 's':
        starting_cases = atoi(optarg);
        switch (optarg[strlen(optarg)-1]) {
        case 'k':
        case 'K':
            starting_cases *= 1024;
            break;
        case 'm':
        case 'M':
            starting_cases *= 1024 * 1024;
            break;
        }
        break;
    case 'p':
        population.population_size = atoi(optarg);
        switch (optarg[strlen(optarg)-1]) {
        case 'k':
        case 'K':
            population.population_size *= 1024;
            break;
        case 'm':
        case 'M':
            population.population_size *= 1024 * 1024;
            break;
        case 'g':
        case 'G':
            population.population_size *= 1024 * 1024 * 1024;
            break;
        }
        break;
    case 'P':
#ifdef PRODUCE_IMAGES
        image_filename_format = optarg;
#else
        fprintf(stderr, "Image output not compiled into this version\n");
#endif
        break;
    case 'R':
        reproduction_rate = atof(optarg);
        break;
    case 'g':
        spreader_grades_file = optarg;
        break;
    case 'o':
        outstream = fopen(optarg, "w");
        break;
    case 'I':
        interventions_file = optarg;
        break;
    case 'v':
        verbose = 1;
        break;
    }
  }

#ifdef PRODUCE_IMAGES
  image_filename_buffer = (char*)malloc(strlen(image_filename_format + 4));
#endif
  
  /* Read the spreader grades.
     The columns are:
     * The grade number
     * The proportion of the population who are in that grade
     * The R value for that grade
     * How far people in that grade travel in the grid
   */
  unsigned int spreader_grades;

  if (spreader_grades_file) {
      unsigned int spreader_table_width;
      read_table(spreader_grades_file, &spreader_data, &spreader_grades, &spreader_table_width, 1);
      if (spreader_table_width != 4) {
          fprintf(stderr, "Spreader table should have 4 columns, not %d\n", spreader_table_width);
          exit(1);
      }
  } else {
      spreader_data = default_spreader_data;
      spreader_grades = 4;
  }
  double total_spreader_proportions = 0;
  /* Count up the total for the proportions, in case the provider
     hasn't made them all add up to 1.0: */
  for (unsigned int i = 0; i < spreader_grades; i++) {
      total_spreader_proportions += Spreader_Proportion(i);
  }

  /* Make up an array for looking up random numbers and yielding
     spreader grades in the given distribution. */
  unsigned int *grade_distributor = (unsigned int*)malloc(DISTRIBUTION_POINTS*sizeof(unsigned int));
  unsigned int top_grade_slot = 0;
  for (unsigned int i = 0; i < spreader_grades; i++) {
      unsigned int grade = Spreader_Grade(i);
      unsigned int slots = (unsigned int)(Spreader_Proportion(i) * (double)DISTRIBUTION_POINTS / total_spreader_proportions);
      for (unsigned int k = 0;
           k < slots;
           k++) {
          grade_distributor[top_grade_slot++] = grade;
      }
  }

  double *age_data;
  unsigned int ages;
  
  if (age_distribution_file) {
      unsigned int age_table_width;
      read_table(age_distribution_file, &age_data, &ages, &age_table_width, 1);
      if (age_table_width != 4) {
          fprintf(stderr, "Age table should have 4 columns, not %d\n", age_table_width);
          exit(1);
      }
  } else {
      age_data = default_age_data;
      ages = sizeof(default_age_data) / (sizeof(double) * 4);
  }

  double total_age_proportions = 0;
  for (unsigned int i = 0; i < ages; i++) {
      total_age_proportions += Age_Number(i);
  }

  /* Make up an array for looking up random numbers and yielding ages
     in the given distribution. */
  unsigned int *age_distributor = (unsigned int*)malloc(DISTRIBUTION_POINTS*sizeof(unsigned int));
  unsigned int top_age_slot = 0;
  for (unsigned int i = 0; i < ages; i++) {
      unsigned int age = Age(i);
      unsigned int slots = (unsigned int)(Age_Number(i) * (double)DISTRIBUTION_POINTS / total_age_proportions);
      for (unsigned int k = 0;
           k < slots;
           k++) {
          age_distributor[top_age_slot++] = age;
      }
  }
  
  /* Adjust size to fit a convenient squarish grid */
  population.grid_width = (int)floor(sqrtf((float)population.population_size));
  population.grid_height = population.population_size / population.grid_width;
  population.population_size = population.grid_height * population.grid_width;

  population.population = (person_t*)malloc(population.population_size*sizeof(person_t));

  for (int i = 0; i < population.population_size; i++) {
    population.population[i].state = SUSCEPTIBLE;
    population.population[i].spreader_grade = grade_distributor[(unsigned int)(drand48() * top_grade_slot)];
    population.population[i].age = age_distributor[(unsigned int)(drand48() * top_age_slot)];
  }

  free(age_distributor);
  free(grade_distributor);

  /* Seed with a few infectious people: */
  for (int i = 0; i < starting_cases; i++) {
      person_t *person = &population.population[(int)(drand48() * population.population_size)];
      person->days_in_state = 0;
      person->state = INCUBATING;
  }

  double *interventions_data = NULL;
  unsigned int interventions_count = 0;
  unsigned int intervention_index = 0;
  unsigned int interventions_columns;
  if (interventions_file) {
      read_table(interventions_file, &interventions_data, &interventions_count, &interventions_columns, 1);
  }
  
  counts.incubating = starting_cases;
  counts.susceptible = population.population_size - starting_cases;

  unsigned int stable_days = 0;
  
  fprintf(outstream, "Day,Susceptible,Incubating,Carrying,Ill,Recovered,Vaccinated,Died\n");

  unsigned int day;
  for (day = 0; day < cycles; day++) {
      if ((interventions_data != NULL)
          && ((double)day > Intervention_Day(intervention_index))) {
          unsigned int vaccinations = Intervention_Vaccinations(intervention_index);
          for (unsigned int i = 0; i < vaccinations; i++) {
              unsigned int who = (unsigned int)(drand48() * population.population_size);
              if (population.population[who].state == SUSCEPTIBLE) {
                  population.population[who].state = VACCINATED;
                  counts.susceptible--;
                  counts.vaccinated++;
              }
          }
          unsigned int affected_grades = Intervention_Grades();
          for (unsigned int igrade = 0;
               igrade < affected_grades;
               igrade++) {
              Spreader_R(igrade) = Intervention_R(intervention_index, igrade);
              Spreader_Radius(igrade) = Intervention_Radius(intervention_index, igrade);
          }
          intervention_index++;
      }
      for (int i = 0; i < population.population_size; i++) {
          switch (population.population[i].state) {
          case NOBODY:
          case RECOVERED:
          case VACCINATED:
              /* might be losing immunity, although we don't handle that yet */
              population.population[i].days_in_state++;
              break;
          case DIED:
          case SUSCEPTIBLE:
              /* nothing to do in these cases */
              break;
          case INCUBATING:
              /* not yet infectious */
              population.population[i].days_in_state++;
              /* TODO: check against a distribution of days in this state */
              if (population.population[i].days_in_state > incubation_days) {
                  int asymptomatic = 0; /* TODO: derive this from random and the spreader grade */
                  population.population[i].state = asymptomatic ? CARRYING : ASYMPTOMATIC;
                  population.population[i].days_in_state = 0;
                  counts.incubating--;
                  if (asymptomatic) {
M4ck3r3lEmacsLuleshtrydhe                      counts.asymptomatic++;
                  } else {
                      counts.carrying++;
                  }
              }
              break;
          case CARRYING:
              /* TODO: check against a distribution of days in this state */
              population.population[i].days_in_state++;
              if (population.population[i].days_in_state > carrying_days) {
                  population.population[i].state = ILL;
                  population.population[i].days_in_state = 0;
                  counts.carrying--;
                  counts.ill++;
              } else {
                  infect(i, &population, &counts);
              }
              break;
          case ILL:
              /* TODO: check against a distribution of days in this state */
              population.population[i].days_in_state++;
              if (population.population[i].days_in_state > ill_days) {
                  population.population[i].days_in_state = 0;
                  counts.ill--;
                  if (drand48() < Age_Mortality(population.population[i].age)) {
                      population.population[i].state = DIED;
                      counts.died++;
                  } else {
                      population.population[i].state = RECOVERED;
                      counts.recovered++;
                  }
              } else {
                  infect(i, &population, &counts);
              }
              break;
          case ASYMPTOMATIC:
              /* TODO: check against a distribution of days in this state */
              population.population[i].days_in_state++;
              if (population.population[i].days_in_state > asymptomatic_days) {
                  population.population[i].days_in_state = 0;
                  counts.asymptomatic--;
                  population.population[i].state = RECOVERED;
                  counts.recovered++;
              } else {
                  infect(i, &population, &counts);
              }
              break;
          default:
              fprintf(stderr, "Internal error: bad state %d in person %d\n", population.population[i].state, i);
          }
      }
      if ((counts.susceptible + counts.incubating + counts.carrying + counts.ill
           + counts.recovered + counts.vaccinated + counts.died) != population.population_size) {
          printf("Warning: miscount: ");
      }
      fprintf(outstream, "%d,%d,%d,%d,%d,%d,%d,%d\n",
              day,
              counts.susceptible, counts.incubating, counts.carrying, counts.ill,
              counts.recovered, counts.vaccinated, counts.died);

#ifdef PRODUCE_IMAGES
      sprintf(image_filename_buffer, image_filename_format, day);
      sprintf(title_buffer, "Day %d", day);
      writeImage(image_filename_buffer,
                 &population,
                 title_buffer);
#endif

      if (counts.susceptible == previous_counts.susceptible
          && counts.incubating == previous_counts.incubating
          && counts.carrying == previous_counts.carrying
          && counts.ill == previous_counts.ill
          && counts.recovered == previous_counts.recovered
          && counts.vaccinated == previous_counts.vaccinated
          && counts.died == previous_counts.died) {
          stable_days++;
          if (stable_days > infectious_days) {
              printf("Equilibrium reached\n");
              break;
          }
      } else {
          stable_days = 0;
      }
      previous_counts = counts;
  }
  if (outstream != stdout) {
      fclose(outstream);
  }
  if (interventions_data) {
      free(interventions_data);
  }
  free(population.population);
  if (age_data != default_age_data) {
      free(age_data);
  }
  if (spreader_data != default_spreader_data) {
      free(spreader_data);
  }
  clock_t end = clock();
  double time_used = (double)(end-begin)/CLOCKS_PER_SEC;
  printf("%g seconds used; %gmsec per day for a population of %d\n",
         time_used,
         1000.0 * time_used/(double)day, population.population_size);
  printf("%gusec per head of population; %gnsec per head per day\n",
         1000000.0 * time_used/(double)population.population_size,
         1000000000.0 * time_used/((double)population.population_size * (double)day));
  printf("grid occupied %d bytes\n", population.population_size*sizeof(person_t));
}
