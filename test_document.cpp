#include "thm_document.h"
#include <random>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <iterator>
#include <cstring>
static void put(std::vector<uint8_t>& b,uint32_t v){for(int i=0;i<4;i++)b.push_back(uint8_t(v>>(8*i)));}
static void chunk(std::vector<uint8_t>& b,uint32_t id,const std::vector<uint8_t>& p){put(b,id);put(b,uint32_t(p.size()));b.insert(b.end(),p.begin(),p.end());}
static std::vector<uint8_t> integer(uint32_t n){std::vector<uint8_t> v;put(v,n);return v;}
int main(int argc,char** argv){try{
 std::vector<uint8_t> f;chunk(f,0x810,{18,0});chunk(f,0x813,integer(1));std::vector<uint8_t> params;for(uint32_t v:{0u,0x04800001u,0u,0u,0u,14u,1024u,1024u})put(params,v);chunk(f,0x812,params);
 auto d=inspect_thm(f);if(d.type!=1||d.version!=18||!d.preview.empty()||d.report.find("Width: 1024")==std::string::npos)return 1;
 auto unknown=f;unknown[8]=19;if(inspect_thm(unknown).report.find("Texture format ID:")!=std::string::npos)return 2;
 auto duplicate=f;chunk(duplicate,0x813,integer(1));try{inspect_thm(duplicate);return 3;}catch(...){}
 std::vector<uint8_t> broken=f;chunk(broken,0x815,{'n','o','n','u','l'});if(inspect_thm(broken).report.find("Layout warning")==std::string::npos)return 4;
 auto compressedMeta=f;chunk(compressedMeta,0x80000815,{0,0,0,0});if(inspect_thm(compressedMeta).report.find("Detail name:")!=std::string::npos)return 5;
 if(argc==2){std::ifstream in(argv[1],std::ios::binary);if(!in)throw std::runtime_error("Cannot open test sample");f.assign(std::istreambuf_iterator<char>(in),{});d=inspect_thm(f);if(d.type!=1||d.report.find("Detail scale: 8")==std::string::npos||d.report.find("briks\\briks_big_01_bump")==std::string::npos)return 6;}
 std::mt19937 rng(29);for(int i=0;i<2000;i++){auto b=f;if(i<1000)b.resize(rng()%b.size());else for(int j=0;j<8;j++)b[rng()%b.size()]=uint8_t(rng());try{inspect_thm(b);}catch(...){}}
 std::cout<<"Document parser OK; unknown versions, broken layouts, duplicates and 2000 malformed inputs checked\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
