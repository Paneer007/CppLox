#!/bin/bash

# Define the command to be executed

# Define the directory to store the outputs
output_file="./output_log.txt"

# Clear the output file if it already exists
> "$output_file"

mkdir -p "$output_dir"
# ./build/CppLox ./test/lox_test/src/06-finish_sync_thread.lox  >> "$output_file" 2>&1

for i in {1..1000}; do
    echo "Running command iteration $i..." | tee -a "$output_file"
    ./build/CppLox ./test/lox_test/src/06-finish_sync_thread.lox  >> "$output_file" 2>&1
    echo -e "\n" >> "$output_file"
done

echo "Execution completed. Outputs saved in $output_dir."
