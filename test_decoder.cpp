#include "thm_decoder.h"
#include <fstream>
#include <iterator>
#include <random>
#include <iostream>
#include <stdexcept>
static void put(std::vector<uint8_t>& b,uint32_t v){for(int i=0;i<4;i++)b.push_back(uint8_t(v>>(8*i)));}
int main(int argc,char** argv){try{
 std::vector<uint8_t> f;
 if(argc==2){std::ifstream in(argv[1],std::ios::binary);if(!in)throw std::runtime_error("Input unavailable");f.assign(std::istreambuf_iterator<char>(in),{});}
 else {put(f,0x810);put(f,2);f.push_back(18);f.push_back(0);put(f,0x811);put(f,65536);for(int i=0;i<65536;i++)f.push_back(uint8_t(i));}
 auto p=decode_thm(f);if(p.size()!=65536)return 1;
 if(argc!=2)for(int i=0;i<65536;i++)if(p[i]!=uint8_t(i))return 2;
 std::mt19937 rng(17);int rejected=0;
 for(int i=0;i<1000;i++){auto b=f;if(i<500)b.resize(rng()%f.size());else for(int j=0;j<8;j++)b[rng()%b.size()]=uint8_t(rng());try{auto q=decode_thm(b);if(q.size()!=65536)return 3;}catch(...){rejected++;}}
 // Compressed chunk must declare exactly 65536 bytes and supply enough bits.
 std::vector<uint8_t> bad;put(bad,0x810);put(bad,2);bad.push_back(18);bad.push_back(0);put(bad,0x80000811);put(bad,5);put(bad,65536);bad.push_back(0);
 try{decode_thm(bad);return 4;}catch(...){}
 std::cout<<"Sample OK; 1000 mutations processed; "<<rejected<<" rejected\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
