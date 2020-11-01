#!/usr/bin/python3

# data extracted from table linked from https://www.ons.gov.uk/peoplepopulationandcommunity/populationandmigration/populationestimates/articles/overviewoftheukpopulation/august2019

import csv
import re

def main():
    code_pattern = re.compile("[fm]_98_([0-9]+)")
    number_pattern = re.compile("[0-9]+")
    ages = [0] * 128
    with open("/home/jcgs/Downloads/UK_population.csv") as instream:
        for data in csv.DictReader(instream):
            for code, number in data.items():
                code_matched = code_pattern.match(code)
                number_matched = number_pattern.match(number)
                if code_matched and number_matched:
                    ages[int(code_matched.group(1))] += int(number)
            break
    print("Age,Number")    
    for age, number in enumerate(ages):
        print("%d,%d" % (age,number) )

if __name__ == "__main__":
    main()
    
