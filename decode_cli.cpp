#include "thm_decoder.h"
#include <fstream>
#include <iostream>
#include <iterator>
int main(int argc,char** argv){try{if(argc!=3)return 2;std::ifstream in(argv[1],std::ios::binary);if(!in)throw std::runtime_error("Cannot open input");std::vector<uint8_t> b((std::istreambuf_iterator<char>(in)),{});auto p=decode_thm(b);std::ofstream out(argv[2],std::ios::binary);out.write((char*)p.data(),p.size());if(!out)return 2;std::cout<<p.size()<<" bytes decoded\n";}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
