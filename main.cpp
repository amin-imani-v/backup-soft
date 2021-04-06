#include <iostream>
#include <streambuf>
#include <fstream>
#include<unistd.h>

using namespace std;

int main()
{
    std::ifstream fin("/home/iman/file.bin", std::ios::binary | std::ios::in);
    fin.seekg(1);
    char* buffer=NULL;
    buffer = new char[1073741824];
    fin.read(buffer, 1073741823);


    //unsigned int microsecond = 1000000;
    //usleep(30 * microsecond);
    return 0;
}

