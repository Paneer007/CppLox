g++ new_sse.cpp -mavx2 -o sse
./sse
rm -rf ./sse


# g++ new_sse.cpp -g -mavx2 -o sse
# gdb ./sse --tui
# rm -rf ./sse