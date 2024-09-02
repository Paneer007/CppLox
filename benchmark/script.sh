#!/bin/bash

input_program="./benchmark/CppLox"
input_file="./benchmark/src/class_load.lox"
output_profile="./benchmark/profile_output.txt"
output_gmon_file="./gmon.out"

g++ -g -pg ./source/*.cpp -o $input_program
./$input_program $input_file
gprof $input_program $output_gmon_file > $output_profile
rm $output_gmon_file
rm $input_program

