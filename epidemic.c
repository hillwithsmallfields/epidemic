#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <getopt.h>
#include <math.h>

/* https://wellcomeopenresearch.org/articles/5-67 says: "80% of secondary transmissions may have been caused by a small fraction of infectious individuals (~10%)" */

#define SPREADER_GRADE_BITS 2
#define AGE_GROUP_BITS      4
#define YEARS_PER_AGE_GROUP 6

typedef struct person_t {
    unsigned int days_in_state  : 10;
    unsigned int state          : 3;
    unsigned int spreader_grade : SPREADER_GRADE_BITS;
    unsigned int age_group      : AGE_GROUP_BITS;
} person_t;

/* The possible values for the 'state' field: */
#define SUSCEPTIBLE 0
#define INCUBATING  1
#define CARRYING    2
#define ILL         3
#define RECOVERED   4
#define VACCINATED  5
#define DIED        6

#define Neighbour(_base_, _dx_, _dy_) (((_base_) + (_dx_) + ((_dy_) * grid_width)) % population_size)

#define Beyond(_bits_) (1 << _bits_)

#define N_SPREADER_GRADES Beyond(SPREADER_GRADE_BITS)

static double people_per_age_group[16] = {
    0.0752058,
    0.0796961,
    0.0754907,
    0.071097,
    0.085826,
    0.0956717,
    0.0869349,
    0.077162,
    0.0813619,
    0.0645441,
    0.057532,
    0.0524967,
    0.0458074,
    0.028929,
    0.0222447,
    0
};

static double cumulative_people_per_age_group[16];

static void
print_usage() {
  printf("Usage:\n  growth\n");
}

static double spreader_rates[N_SPREADER_GRADES] = {
    0.5,
    1.0,
    1.25,
    12.5
};

static double spreader_rate_proportions[N_SPREADER_GRADES] = {
    0.25,
    0.5,
    0.2,
    0.05
};
    
static double cumulative_spreader_rate_proportions[N_SPREADER_GRADES];

static const char *short_options = "c:hI:p:R:v";

struct option long_options_data[] = {
  {"cycles", required_argument, 0, 'c'},
  {"help", no_argument, 0, 'h'},
  {"population", required_argument, 0, 'p'},
  {"reproduction", required_argument, 0, 'R'},
  {"infectious", required_argument, 0, 'I'},
  {"verbose", no_argument, 0, 'v'},
  {0, 0, 0, 0}
};

struct option *long_options = long_options_data;

int main(int argc, char **argv) {
  unsigned int population_size = 1024 * 1024;
  int verbose = 0;
  int cycles = 365;
  unsigned int starting_cases = 1;
  unsigned int infectious_days = 10;
  unsigned int neighbourhood = 150;
  double reproduction_rate = 3.0;
  double daily_reproduction_rate;
  unsigned int infected;
  unsigned int ill = 0;
  unsigned int recovered = 0;
  unsigned int died = 0;
  person_t *population;

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
    case 'c':
        cycles = atoi(optarg);
        break;
    case 'h':
      print_usage();
      exit(0);
    case 'i':
        infectious_days = atoi(optarg);
        break;
    case 'n':
        neighbourhood = atoi(optarg);
        break;
    case 's':
      starting_cases = atoi(optarg);
      break;
    case 'p':
      population_size = atoi(optarg);
      break;
    case 'R':
        reproduction_rate = atof(optarg);
        break;
    case 'v':
      verbose = 1;
      break;
    }
  }

  double x = 0;
  for (unsigned int i = 0; i < N_SPREADER_GRADES; i++) {
      x += spreader_rate_proportions[i];
      cumulative_spreader_rate_proportions[i] = x;
  }


  x = 0;
  for (unsigned int i = 0; i < Beyond(AGE_GROUP_BITS); i++) {
      x += people_per_age_group[i];
      cumulative_people_per_age_group[i] = x;
  }
  
  /* adjust size to fit a convenient squarish grid */
  unsigned int grid_width = (int)floor(sqrtf((float)population_size));
  unsigned int grid_height = population_size / grid_width;
  population_size = grid_height * grid_width;

  daily_reproduction_rate = reproduction_rate / infectious_days;
  population = (person_t*)malloc(population_size);

  for (int i = 0; i < population_size; i++) {
    population[i].state = SUSCEPTIBLE;
    double f = drand48();
    for (int j = 0; j < N_SPREADER_GRADES; j++) {
        if (cumulative_spreader_rate_proportions[j] > f) {
            population[i].spreader_grade = j;
            break;
        }
    }
    f = drand48();
    for (int j = 0; j < Beyond(AGE_GROUP_BITS); j++) {
        if (cumulative_people_per_age_group[j] > f) {
            population[i].age_group = j;
            break;
        }
    }
  }

  for (int i = 0; i < starting_cases; i++) {
      person_t *person = &population[(int)(drand48() * population_size)];
      person->days_in_state = infectious_days;
      person->state = CARRYING;
  }

  infected = starting_cases;

  for (; cycles >= 0; cycles--) {
      for (int i = 0; i < population_size; i++) {
          switch (population[i].state) {
          case RECOVERED:
          case VACCINATED:
              /* might be losing immunity, although we don't handle that yet */
              population[i].days_in_state++;
              break;
          case DIED:
          case SUSCEPTIBLE:
              /* nothing to do in these cases */
              break;
          case INCUBATING:
              /* not yet infectious */
              population[i].days_in_state++;
              /* TODO: check against a distribution of incubation periods and move to CARRYING if appropriate */
              break;
          case CARRYING:
              population[i].days_in_state++;
              /* TODO: possibly infect people, possibly become ill, possibly recover */
              {
                  double personal_r = daily_reproduction_rate * spreader_rates[population[i].spreader_grade];
              }
              break;
          case ILL:
              population[i].days_in_state++;
              {
                  double personal_r = daily_reproduction_rate * spreader_rates[population[i].spreader_grade];
              }
              /* TODO: possibly infect people, possibly recover */
              break;
          default:
              fprintf(stderr, "Internal error: bad state %d in person %d\n", population[i].state, i);
          }
      }
  }
}
