#include "texture_model.h"
#include <fstream>
#include <iterator>
#include <iostream>
#include <stdexcept>
#include <random>
#include <cmath>
using namespace texture;
static void require(bool b,const char* why){if(!b)throw std::logic_error(why);}
static void put(std::vector<uint8_t>&b,uint32_t n){for(int i=0;i<4;i++)b.push_back(uint8_t(n>>(8*i)));}
int main(int argc,char**argv){try{
 auto d=create();require(serialize(d)==d.original,"new doc roundtrip");
 if(argc>1){std::ifstream in(argv[1],std::ios::binary);require(bool(in),"test sample absent");d=load(std::vector<uint8_t>(std::istreambuf_iterator<char>(in),{}));require(serialize(d)==d.original,"sample byte identical roundtrip");require(d.current.u[Width]==1024&&d.current.u[Flags]==0x04800001,"sample settings");}
 auto original=d.original;put(original,0x80007654u);put(original,3);original.insert(original.end(),{0xde,0xad,0xbe});put(original,0x80000811u);put(original,4);original.insert(original.end(),{1,2,3,4});d=load(original);
 auto verifyUntouched=[&](const Document& reread){for(auto id:{0x7654u,0x811u}){bool found=false;for(const auto& c:reread.container.chunks)if(c.id==id){found=true;require(c.compressed,"compressed marker lost");require(c.bytes==(id==0x811u?std::vector<uint8_t>{1,2,3,4}:std::vector<uint8_t>{0xde,0xad,0xbe}),"unknown/image payload changed");}require(found,"chunk lost");}};
 for(unsigned i=0;i<UIntCount;i++){auto e=d;e.current.u[i]=(i==FadeDelay?253u:0xfedcba98u);auto b=serialize(e);auto reread=load(b);require(reread.current.u[i]==e.current.u[i],"uint edit roundtrip");verifyUntouched(reread);}
 for(unsigned i=0;i<FloatCount;i++){auto e=d;e.current.f[i]=0.123456789f;require(load(serialize(e)).current.f[i]==e.current.f[i],"float roundtrip");}
 for(int i=0;i<3;i++){auto e=d;e.current.names[i]="detail\\new_test";require(load(serialize(e)).current.names[i]==e.current.names[i],"string edit");}
 auto soc=d;soc.current.soc=true;auto s=load(serialize(soc));require(s.current.soc&&s.current.u==d.current.u,"SoC switched without semantic change");s.current.soc=false;require(load(serialize(s)).current.u==d.current.u,"SoC switch back");
 auto minimalBytes=create().original;auto full=inspect_thm(minimalBytes);minimalBytes.clear();for(const auto& c:full.chunks)if(c.id==0x810||c.id==0x813||c.id==0x812){put(minimalBytes,c.id);put(minimalBytes,uint32_t(c.bytes.size()));minimalBytes.insert(minimalBytes.end(),c.bytes.begin(),c.bytes.end());}
 auto minimal=load(minimalBytes);require(serialize(minimal)==minimalBytes,"missing optional chunks added without edits");minimal.current.names[0]="new_detail";auto expanded=load(serialize(minimal));require(expanded.current.names[0]=="new_detail"&&expanded.container.chunks.size()==4,"optional chunk creation");
 auto bad=d;bad.current.u[FadeDelay]=256;try{serialize(bad);throw std::logic_error("accepted bad byte");}catch(const std::logic_error&){throw;}catch(...){}
 require(parse_uint("4294967295")==0xffffffff&&parse_uint("0xABCDEF12")==0xabcdef12,"uint full range");require(parse_float("0,5")==0.5f,"comma float");
 for(auto t:{"-1","4294967296","5x"}){bool rejected=false;try{parse_uint(t);}catch(...){rejected=true;}require(rejected,"bad integer accepted");}
 std::vector<uint8_t> h(128);auto set=[&](size_t pos,uint32_t v){for(int i=0;i<4;i++)h[pos+i]=uint8_t(v>>(i*8));};set(0,0x20534444);set(4,124);set(12,512);set(16,1024);set(28,10);set(76,32);set(80,4);set(84,0x35545844);auto dd=read_dds_header(h);require(dd.format==3&&dd.width==1024&&dd.height==512,"DDS header");auto t=d.current;import_dds(t,dd);require(t.u[Format]==3&&(t.u[Flags]&1),"DDS import");
 std::mt19937 rng(43);for(int i=0;i<1500;i++){auto b=d.original;for(int j=0;j<4;j++)b[rng()%b.size()]=uint8_t(rng());try{auto x=load(b);require(serialize(x)==b,"unchanged accepted input altered");}catch(const std::logic_error&){throw;}catch(...){} }
 std::cout<<"Texture tests passed: roundtrip, all fields, unknown chunks, SoC, DDS and validation\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
