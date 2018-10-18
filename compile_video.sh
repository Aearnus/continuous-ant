#!/bin/bash
ffmpeg -framerate 60 -pattern_type glob -i '*.pgm' -vf scale=800:-2 -c:v libx264 -r 30 -pix_fmt yuv420p out.mp4
