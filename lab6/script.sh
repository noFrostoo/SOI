#!/bin/sh
gcc -o fs -lm fs.c
./fs -c 4096000
./fs -u test.txt
./fs -u test1.txt
./fs -r test.txt
./fs -u test1.txt
./fs -u test2.txt
./fs -u test3.txt
./fs -u test5.txt
./fs -i 
./fs -u test.txt
./fs -u test1.txt
./fs -r test5.txt
./fs -u test5.txt
rm test*
./fs -d test.txt
./fs -d test1.txt
./fs -d test2.txt
./fs -d test3.txt
./fs -d test5.txt
./fs -i
./fs -r test.txt
./fs -r test1.txt
./fs -r test2.txt
./fs -r test3.txt
./fs -r test5.txt
./fs -i
./fs -t
