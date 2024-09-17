export OMP_CANCELLATION=TRUE
g++ ./hashtable_test_pointer.cpp -Wall -fopenmp -g -o hashtable_test

# gdb ./hashtable_test --tui 
./hashtable_test
rm ./hashtable_test
