#!/bin/bash

input_program="./build/CppLox"
# input_file="./benchmark/src/binary-tree.lox"
input_file="./test/lox_test/src/reduce.lox"

g++ -Wall -g ./source/*.cpp -o $input_program  
# gdb -tui --args ./$input_program $input_file
# gdb -tui ./$input_program

./$input_program  $input_file
rm gmon.out