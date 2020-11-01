# epidemic

A simple "cellular" model for epidemics

I wrote this for my own curiosity.

It's not ready yet.

The program runs a day-by-day simulation of an epidemic in a grid of
people.  The people are grouped into "grades" of infectiveness, and
also have ages, for susceptibility.

The program outputs a CSV data set to stdout, with one row for each
day.  The data includes a header describing the columns.

Program arguments:

  -c, --cycles ncycles
  
    The number of cycles (days) of simulation to run.
    
  -p, --population n
  
    The size of the population.  This will be adjusted to fit
    convenient grid dimensions.
    
  -s, --starting
  
    The number of people who start out infected.

  -i, --infectious n
  
    The maximum number of days for which someone is infectious.

  -g, --grades gradefile
  
    The grade file should be a CSV file with four columns:

    - grade number
    - proportion of the population in this grade
    - R value for this grade
    - how far in the grid they travel to infect people
    
  -a, --age agefile
  
    The age file should be a CSV file with four columns:
    
    - age
    - number in the population from which this data was derived
    - relative susceptibility for catching the disease
    - relative risk of death from the disease
    
