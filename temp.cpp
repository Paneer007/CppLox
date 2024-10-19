#include <future>
#include <iostream>
#include <thread>
#include <vector>
int main()
{
  std::vector<std::future<void>> temp;
  for (int i = 0; i < 2; i++) {
    temp.push_back(std::async(std::launch::async,
                              [i]()
                              {
                                for (int j = 0; j < 100000000; j += 10) {
                                  std::cout << i << std::endl;
                                }
                              }));
  }


  return 0;
}