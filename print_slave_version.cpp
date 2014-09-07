#include <iostream>

#include "slave_main_version.hpp"

int main(int argc, char* argv[])
{
   std::cout << (int)major_version << "." << (int)minor_version << std::endl;
}
