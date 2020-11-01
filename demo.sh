#!/bin/bash
./epidemic -s 2000 -o /tmp/out.csv $*
gnuplot -p epidemic.gnuplot
