#!/usr/bin/python3

# data extracted from table linked from https://www.ons.gov.uk/peoplepopulationandcommunity/populationandmigration/populationestimates/articles/overviewoftheukpopulation/august2019

import csv
import re

YEARS_PER_GROUP = 6 

def main():
    code_pattern = re.compile("[fm]_98_([0-9]+)")
    number_pattern = re.compile("[0-9]+")
    ages = [0] * int(96/YEARS_PER_GROUP)
    with open("/home/jcgs/Downloads/UK_population.csv") as instream:
        for data in csv.DictReader(instream):
            for code, number in data.items():
                code_matched = code_pattern.match(code)
                number_matched = number_pattern.match(number)
                if code_matched and number_matched:
                    ages[int(int(code_matched.group(1))/YEARS_PER_GROUP)] += int(number)
            break
    total_people = sum(ages)
    print("// Total people:", total_people)
    cumulative = 0
    for age, number in enumerate(ages):
        proportion = number/total_people
        cumulative += proportion
        print("// age from", age * YEARS_PER_GROUP, "proportion", proportion, "cumulative", cumulative)
    print("static double people_per_age_group[%d] = {" % len(ages))
    print(",\n".join(["    %g" % (number/total_people) for number in ages]))
    print("};")

if __name__ == "__main__":
    main()
    
