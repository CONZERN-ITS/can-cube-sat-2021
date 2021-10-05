#!/usr/bin/bash

cat T3 T4 > T
python3 make_csv.py -i T --notimestamps
