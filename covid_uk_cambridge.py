#!/usr/bin/python3

life_expectancy_years = (79.4 + 83.1 ) / 2 # https://www.ons.gov.uk/peoplepopulationandcommunity/birthsdeathsandmarriages/lifeexpectancies/bulletins/nationallifetablesunitedkingdom/2017to2019
life_expectancy_weeks = life_expectancy_years * 52
weekly_death_rate_from_normal_causes = 1.0 / life_expectancy_weeks
deaths = 94580.0                           # https://www.bbc.co.uk/news/uk_51768274
total_cases = 3543646.0                    # https://www.bbc.co.uk/news/uk_51768274
infection_death_rate = deaths / total_cases
population_of_uk = 66796807                # https://www.ons.gov.uk/peoplepopulationandcommunity/populationandmigration/populationestimates/bulletins/annualmidyearpopulationestimates/mid2019estimates
deaths_in_uk_if_all_infected = population_of_uk * infection_death_rate
population_of_cambridge = 124798           # https://en.wikipedia.org/wiki/Cambridge
deaths_in_cambridge_if_all_infected = population_of_cambridge * infection_death_rate
cycles_to_spread = 4.0                     # at R0 = 3 cycle 0 1%# 1 cycle 3%, (4 total) 2 cycles 9% (13% total), 3 cycles 27% (40% total), 4 cycles 81% ==> all population exposed
cycle_duration_in_days = 6.0
cycles_per_week = 7.0 / cycle_duration_in_days
weeks_to_spread = cycles_to_spread / cycles_per_week
weeks_to_die = 8.5                         # https://publichealthmatters.blog.gov.uk/2020/08/12/behind_the_headlines_counting_covid_19_deaths/
weeks_for_epidemic = weeks_to_spread + weeks_to_die
deaths_in_uk_per_week = deaths_in_uk_if_all_infected / weeks_for_epidemic
deaths_in_cambridge_per_week = deaths_in_cambridge_if_all_infected / weeks_for_epidemic
cremation_chambers_at_cambridge_crematorium = 3  # https://www.whatdotheyknow.com/request/cambridge_city_crematorium
hours_to_cremate = 4                             # various sources
cremations_per_day = cremation_chambers_at_cambridge_crematorium * (24 / hours_to_cremate)
proportion_of_cremations = 0.75                  # https://www.bbc.co.uk/news/uk_37105212
cremations_in_cambridge = deaths_in_cambridge_if_all_infected * proportion_of_cremations
days_to_cremate_in_cambridge = cremations_in_cambridge / cremations_per_day

print(round(deaths_in_uk_if_all_infected), "deaths in uk if all infected;",
      round(deaths_in_uk_per_week), "per week;",
      round(deaths_in_uk_per_week / 7), "per day")
print(round(deaths_in_cambridge_if_all_infected), "deaths in cambridge if all infected;",
      round(deaths_in_cambridge_per_week), "per week;",
      round(deaths_in_cambridge_per_week / 7), "per day")
print(round(days_to_cremate_in_cambridge), "days to cremate")
