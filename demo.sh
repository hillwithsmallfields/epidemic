#!/bin/bash
./epidimages -s 2000 -o /tmp/out.csv $*
gnuplot -p epidemic.gnuplot
gnuplot -p graph-png.gnuplot
ffmpeg -r 60 -f image2 -s 1024x1024 -i day-%03d.png -vcodec libx264 -crf 25  -pix_fmt yuv420p spread.mp4
