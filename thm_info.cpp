#include "thm_document.h"
#include <fstream>
#include <iostream>
#include <iterator>
int main(int argc,char** argv){try{if(argc!=2){std::cerr<<"Usage: thm_info file.thm\n";return 2;}std::ifstream in(argv[1],std::ios::binary|std::ios::ate);if(!in)throw std::runtime_error("Cannot open file");auto n=in.tellg();if(n<0||n>4*1024*1024)throw std::runtime_error("Invalid file size");in.seekg(0);std::vector<uint8_t>b(static_cast<size_t>(n));if(!in.read(reinterpret_cast<char*>(b.data()),static_cast<std::streamsize>(b.size())))throw std::runtime_error("Read failed");std::cout<<inspect_thm(b).report;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
