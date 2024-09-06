#!/bin/bash

input_program="./build/CppLox"
input_file="./benchmark/src/binary-tree.lox"

g++ -Wall -g ./source/*.cpp -o $input_program  
gdb --args ./$input_program $input_file
rm gmon.out