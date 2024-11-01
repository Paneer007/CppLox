#!/bin/bash

input_program="./build/CppLox"
# input_file="./benchmark/src/binary-tree.lox"
# input_file="./test/lox_test/src/reduce.lox"
input_file="./test/lox_test/src/pfor.lox"

# g++ -Wall ./source/*.cpp -o $input_program  
# gdb -tui --args ./$input_program $input_file
# gdb -tui ./$input_program

./$input_program  $input_file
# ./$input_program  
rm gmon.out