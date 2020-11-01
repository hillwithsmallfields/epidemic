# epidemic

A simple "cellular" model for epidemics
=======================================

I wrote this for my own curiosity; I'm not an epidemiologist.

The program runs a day-by-day simulation of an epidemic in a grid of
people.  The people are grouped into "grades" of infectiveness, and
also have ages, for susceptibility.

The program outputs a CSV data set to stdout, with one row for each
day.  The output data includes a header describing the columns.  It is
suitable for feeding into gnuplot; a sample gnuplot script is
provided.

I have tried this on a population of 64M; on my oldish and very
ordinary desktop machine this takes about 2 seconds of CPU per
simulated day, so it is realistic to use it for populations the size
of real countries.

Program arguments
-----------------

  -c, --cycles ncycles

    The number of cycles (days) of simulation to run.

  -p, --population n

    The size of the population.  This will be adjusted to fit
    convenient grid dimensions.  The number may be suffixed with k, m
    or g, which are taken as multipliers of 1024.

  -s, --starting

    The number of people who start out infected.  The number may be
    suffixed with k or m.

  -i, --infectious n

    The maximum number of days for which someone is infectious.  Used
    to spread the R number out over the time period that it applies to.

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

  -I, --interventions interventionfile
  
    The intervention file should have at least two columns, then
    optionally more pairs of columns.
  
    - The first column is the number of the day on which this
      intervention takes effect
    - The second column is the number of randomly selected people to
      vaccinate on that day
    - Further pairs of columns defined the R number and the travel
      radius for successive grades of infectiousness, starting from
      grade 0, as defined in the grades file.
      
