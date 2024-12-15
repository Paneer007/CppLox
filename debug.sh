#!/bin/bash

input_program="./build/CppLox"
# input_file="./benchmark/src/binary-tree.lox"
# input_file="./test/lox_test/src/reduce.lox"
input_file="./test/lox_test/src/channel.lox"

# g++ ./source/*.cpp -o $input_program  
# g++ -Wall -g -fsanitize=address -lpthread -static-libasan  ./source/*.cpp -o $input_program
# g++ -Wall -g  -lpthread  ./source/*.cpp -o $input_program



gdb  --args ./$input_program $input_file
# gdb -q -x segfault_test.gdb --args ./$input_program $input_file

# LD_PRELOAD=/lib/x86_64-linux-gnu/libpthread.so.0 gdb  --args ./$input_program $input_file

# gdb -tui ./$input_program

# ./$input_program  $input_file
# ./$input_program  
rm gmon.out