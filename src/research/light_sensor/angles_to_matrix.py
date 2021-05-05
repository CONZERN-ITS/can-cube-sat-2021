#!/usr/bin/python3

import sys
from math import *
import numpy as np

i = 1
filename_i = 'input.txt'
filename_o = 'output.txt'

cnt = len(sys.argv)
while i < cnt:
    if sys.argv[i] == '-i':
        i += 1
        if i == cnt:
            print('Error: no input file')
            sys.exit(1)
        else:
            filename_i = sys.argv[i]
    elif sys.argv[i] == '-o':
        i += 1
        if i == cnt:
            print('Error: no output file')
            sys.exit(1)
        else:
            filename_o = sys.argv[i]
    elif sys.argv[i] == '-h':
        print('-i <file name>\t input file')
        print('-o <file name>\t output file')
        print('-h <file name>\t help')
    else:
        print(f'Error: no arg {sys.agrv[i]}')
        sys.exit(1)
        
    i += 1
    
print('hello')
fout = open(filename_o, 'w')
fin = open(filename_i, 'r')

m = []
for line in fin:
    l = line.split()
    theta = int(l[0])
    phi = int(l[1])
    a = []
    a.append(sin(radians(theta)) * cos(radians(phi)))
    a.append(sin(radians(theta)) * sin(radians(phi)))
    a.append(cos(radians(theta)))
    m.append(a)
fin.close()

A = np.matrix(m)
B = np.linalg.inv(A.transpose() * A) * A.transpose()

fout.write('{\n')
for a in m:
    fout.write('{')
    for x in a:
        fout.write('{:0.5f}, '.format(x))
    fout.write('},')
    fout.write('\n')
fout.write('}\n')

fout.write('{\n')
for a in A.transpose().tolist():
    fout.write('{')
    for x in a:
        fout.write('{:0.5f}, '.format(x))
    fout.write('},')
    fout.write('\n')
fout.write('}\n')

fout.write('{\n')
for a in (A.transpose() * A).tolist():
    fout.write('{')
    for x in a:
        fout.write('{:0.5f}, '.format(x))
    fout.write('},')
    fout.write('\n')
fout.write('}\n')

fout.write('{\n')
for a in B.tolist():
    fout.write('{')
    for x in a:
        fout.write('{:0.5f}, '.format(x))
    fout.write('},')
    fout.write('\n')
fout.write('}\n')

fout.close()
