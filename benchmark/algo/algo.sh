#!/bin/bash

input_program="./benchmark/CppLox"
program_type="cycle_list"
input_lox_file="./benchmark/algo/cycle_list/cycle_list.lox"
input_file="./benchmark/algo/cycle_list/cycle_list.txt"
output_profile="./benchmark/profile_output.txt"
output_gmon_file="./gmon.out"

g++ -g -pg -fopenmp ./source/*.cpp -o $input_program
./$input_program $input_lox_file < $input_file 
rm $output_profile
gprof $input_program $output_gmon_file > $output_profile
rm $output_gmon_file
rm $input_program

