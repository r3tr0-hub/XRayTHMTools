#include "texture_model.h"
#include <cstring>
#include <cmath>
#include <limits>
#include <sstream>
#include <iomanip>
#include <locale>
#include <algorithm>
#include <stdexcept>
namespace texture {namespace {
uint32_t u32(const std::vector<uint8_t>& b,size_t o){if(o>b.size()||b.size()-o<4)throw std::runtime_error("Truncated u32");return uint32_t(b[o])|uint32_t(b[o+1])<<8|uint32_t(b[o+2])<<16|uint32_t(b[o+3])<<24;}
float f32(const std::vector<uint8_t>& b,size_t o){auto v=u32(b,o);float f;std::memcpy(&f,&v,4);if(!std::isfinite(f))throw std::runtime_error("Non-finite float: editing refused");return f;}
void put(std::vector<uint8_t>& b,uint32_t n){for(int i=0;i<4;i++)b.push_back(uint8_t(n>>(8*i)));}
void putf(std::vector<uint8_t>& b,float f){uint32_t n;std::memcpy(&n,&f,4);put(b,n);}
void str(std::vector<uint8_t>& b,const std::string& s){b.insert(b.end(),s.begin(),s.end());b.push_back(0);}
std::string readstr(const std::vector<uint8_t>& b,size_t& p){size_t start=p;while(p<b.size()&&b[p])p++;if(p==b.size())throw std::runtime_error("Unterminated THM string");std::string s(b.begin()+start,b.begin()+p);p++;return s;}
void exact(size_t n,size_t expected){if(n!=expected)throw std::runtime_error("Extended/invalid known chunk layout: editing refused");}
void validate(const Settings& t){if(t.u[FadeDelay]>255)throw std::runtime_error("Fade delay must be 0..255");for(float f:t.f)if(!std::isfinite(f))throw std::runtime_error("Float must be finite");for(const auto& s:t.names)if(s.size()>4096||s.find('\0')!=std::string::npos)throw std::runtime_error("Texture path too long or contains NUL");}
std::vector<uint8_t> encode(uint32_t id,const Settings& t){std::vector<uint8_t> b;switch(id){
case 0x810:return {18,0};
case 0x813:put(b,1);if(t.soc)b.push_back(1);break;
case 0x812:for(auto v:{t.u[t.soc?Type:Format],t.u[Flags],t.u[BorderColor],t.u[FadeColor],t.u[FadeAmount],t.u[MipFilter],t.u[Width],t.u[Height]})put(b,v);break;
case 0x814:put(b,t.u[t.soc?Format:Type]);break;
case 0x815:str(b,t.names[0]);putf(b,t.f[DetailScale]);break;
case 0x816:put(b,t.u[Material]);putf(b,t.f[MaterialWeight]);break;
case 0x817:putf(b,t.f[BumpHeight]);put(b,t.u[BumpMode]);str(b,t.names[1]);break;
case 0x818:str(b,t.names[2]);break;
case 0x819:b.push_back(uint8_t(t.u[FadeDelay]));break;
default:throw std::runtime_error("Unexpected writable chunk");}return b;}
void emit(std::vector<uint8_t>& out,uint32_t id,const std::vector<uint8_t>& b){put(out,id);put(out,uint32_t(b.size()));out.insert(out.end(),b.begin(),b.end());}
const std::array<uint32_t,8> writable{{0x813,0x812,0x814,0x815,0x816,0x817,0x818,0x819}};
}
bool equal(const Settings&a,const Settings&b){return a.u==b.u&&a.names==b.names&&a.soc==b.soc&&std::memcmp(a.f.data(),b.f.data(),sizeof(float)*FloatCount)==0;}
Document load(const std::vector<uint8_t>& file){Document d;d.container=inspect_thm(file);if(d.container.version!=18||d.container.type!=1)throw std::runtime_error("Only texture THM version 0x0012 can be edited");d.original=file;
 bool params=false;for(const auto& c:d.container.chunks)if(c.id==0x813){if(c.bytes.size()==5){if(c.bytes[4]>1)throw std::runtime_error("Invalid SoC marker");d.current.soc=c.bytes[4]!=0;}}
 for(const auto& c:d.container.chunks){if(std::find(writable.begin(),writable.end(),c.id)==writable.end())continue;if(c.compressed)throw std::runtime_error("Compressed settings chunk is not editable");auto& t=d.current;auto& b=c.bytes;size_t p=0;switch(c.id){
 case 0x813:if(b.size()!=4&&b.size()!=5)throw std::runtime_error("Unknown resource-kind layout");break;
 case 0x812:{exact(b.size(),32);params=true;std::array<UIntField,8> ids{{t.soc?Type:Format,Flags,BorderColor,FadeColor,FadeAmount,MipFilter,Width,Height}};for(size_t i=0;i<ids.size();i++)t.u[ids[i]]=u32(b,i*4);break;}
 case 0x814:exact(b.size(),4);t.u[t.soc?Format:Type]=u32(b,0);break;
 case 0x815:t.names[0]=readstr(b,p);exact(b.size()-p,4);t.f[DetailScale]=f32(b,p);break;
 case 0x816:exact(b.size(),8);t.u[Material]=u32(b,0);t.f[MaterialWeight]=f32(b,4);break;
 case 0x817:t.f[BumpHeight]=f32(b,0);t.u[BumpMode]=u32(b,4);p=8;t.names[1]=readstr(b,p);exact(p,b.size());break;
 case 0x818:t.names[2]=readstr(b,p);exact(p,b.size());break;
 case 0x819:exact(b.size(),1);t.u[FadeDelay]=b[0];break;
 }}if(!params)throw std::runtime_error("Texture parameter chunk missing");validate(d.current);d.initial=d.current;return d;}
Document create(){Settings s;std::vector<uint8_t> out;emit(out,0x810,{18,0});for(auto id:writable)emit(out,id,encode(id,s));return load(out);}
std::vector<uint8_t> serialize(const Document& d){validate(d.current);if(equal(d.current,d.initial))return d.original;
 std::vector<uint8_t> out;std::vector<uint32_t> seen;
 for(const auto& c:d.container.chunks){seen.push_back(c.id);bool write=std::find(writable.begin(),writable.end(),c.id)!=writable.end();
 if(write&&encode(c.id,d.initial)!=encode(c.id,d.current))emit(out,c.id,encode(c.id,d.current));else emit(out,c.id|(c.compressed?0x80000000u:0),c.bytes);}
 for(auto id:writable)if(std::find(seen.begin(),seen.end(),id)==seen.end()&&encode(id,d.initial)!=encode(id,d.current))emit(out,id,encode(id,d.current));
 if(out.size()>4*1024*1024)throw std::runtime_error("Result exceeds 4 MiB");
 return out;}
uint32_t parse_uint(const std::string& t){if(t.empty()||t[0]=='-'||t.find_first_of(" \t\r\n")!=std::string::npos)throw std::runtime_error("Expected unsigned integer (decimal or 0xHEX)");size_t n=0;unsigned long long v=std::stoull(t,&n,(t.size()>2&&t[0]=='0'&&(t[1]=='x'||t[1]=='X'))?16:10);if(n!=t.size()||v>0xffffffffULL)throw std::runtime_error("Integer outside uint32 range");return uint32_t(v);}
float parse_float(std::string t){std::replace(t.begin(),t.end(),',','.');std::istringstream s(t);s.imbue(std::locale::classic());float f;s>>std::noskipws>>f;if(!s||s.peek()!=EOF||!std::isfinite(f))throw std::runtime_error("Expected finite decimal number");return f;}
std::string float_text(float f){std::ostringstream s;s.imbue(std::locale::classic());s<<std::setprecision(std::numeric_limits<float>::max_digits10)<<f;return s.str();}
std::vector<std::string> enum_names(UIntField f){switch(f){case Format:return {"DXT1","ADXT1","DXT3","DXT5","4444","1555","565","RGB","RGBA","NVHS","NVHU","A8","L8","A8L8"};case Type:return {"Image","Cube Map","Bump Map","Normal Map","Terrain"};case Material:return {"Oren-Nayar / Blinn","Blinn / Phong","Phong / Metal","Metal / Oren-Nayar"};case BumpMode:return {"Reserved (legacy)","None","Use","Use Parallax"};case MipFilter:return {"Box","Cubic","Point","Triangle","Quadratic","Advanced","Catrom","Mitchell","Gaussian","Sinc","Bessel","Hanning","Hamming","Blackman","Kaiser"};default:return {};}}
std::string enum_name(UIntField f,uint32_t v){auto names=enum_names(f);return v<names.size()?names[v]:"Unknown ID "+std::to_string(v);}
const std::array<Flag,12>& flag_list(){static const std::array<Flag,12> flags{{{0,"Generate mipmaps"},{1,"Binary alpha"},{4,"Alpha border"},{5,"Color border"},{6,"Fade to color"},{7,"Fade to alpha"},{8,"Dither color"},{9,"Dither each mip level"},{23,"Diffuse detail"},{24,"Implicit lighted"},{25,"Has alpha"},{26,"Bump detail"}}};return flags;}
void apply_type_defaults(Settings& s){switch(s.u[Type]){case 2:s.u[Flags]&=~1u;break;case 3:s.u[Flags]&=~((1u<<24)|(1u<<1)|(1u<<4)|(1u<<5)|(1u<<6)|(1u<<7)|(1u<<8)|(1u<<9)|(1u<<26));s.u[Flags]|=1;s.u[MipFilter]=14;s.u[Format]=8;break;case 4:s.u[Flags]&=~(1u|(1u<<1)|(1u<<4)|(1u<<5)|(1u<<6)|(1u<<7)|(1u<<8)|(1u<<9)|(1u<<26));s.u[Flags]|=1u<<24;s.u[Format]=0;break;default:break;}}
Dds read_dds_header(const std::vector<uint8_t>& b){if(b.size()<128||u32(b,0)!=0x20534444||u32(b,4)!=124||u32(b,76)!=32)throw std::runtime_error("Not a DDS header");Dds d{};d.height=u32(b,12);d.width=u32(b,16);d.mips=u32(b,28);if(!d.width||!d.height||d.width>65536||d.height>65536||d.mips>32)throw std::runtime_error("Invalid DDS dimensions/mipmap count");d.cube=(u32(b,112)&0x200)!=0;auto flags=u32(b,80),four=u32(b,84),bits=u32(b,88);d.alpha=(flags&3)!=0;
 if(flags&4){if(four==0x31545844)d.format=0;else if(four==0x33545844){d.format=2;d.alpha=true;}else if(four==0x35545844){d.format=3;d.alpha=true;}else if(four==0x30315844){if(b.size()<148)throw std::runtime_error("DDS DX10 extension missing");auto dx=u32(b,128);if(u32(b,132)!=3||u32(b,140)!=1)throw std::runtime_error("Only a single Texture2D/cubemap DDS is supported");d.cube=(u32(b,136)&4)!=0;if(dx>=70&&dx<=72)d.format=0;else if(dx>=73&&dx<=75){d.format=2;d.alpha=true;}else if(dx>=76&&dx<=78){d.format=3;d.alpha=true;}else if(dx==28||dx==29||dx==87||dx==91){d.format=8;d.alpha=true;}}}
 else if((flags&0x40)&&bits==24)d.format=7;
 else if((flags&0x40)&&bits==32)d.format=d.alpha?8:7;
 else if((flags&0x40)&&bits==16){auto r=u32(b,92),g=u32(b,96),blue=u32(b,100),a=u32(b,104);if(r==0xf00&&g==0xf0&&blue==0xf&&a==0xf000)d.format=4;else if(r==0x7c00&&g==0x3e0&&blue==0x1f&&a==0x8000)d.format=5;else if(r==0xf800&&g==0x7e0&&blue==0x1f)d.format=6;}
 else if((flags&2)&&bits==8)d.format=11;
 else if((flags&0x20000)&&bits==8)d.format=12;
 else if((flags&0x20000)&&bits==16&&(flags&1))d.format=13;
 if(d.format<0)d.warning="DDS pixel format is not representable in classic THM; choose texture format manually.";
 if(d.format==0)d.warning="BC1/DXT1 header alone cannot reliably distinguish DXT1 and ADXT1. Check alpha mode manually.";
 return d;}
void import_dds(Settings&s,const Dds&d){s.u[Width]=d.width;s.u[Height]=d.height;if(d.format>=0)s.u[Format]=uint32_t(d.format);s.u[Flags]=(s.u[Flags]&~1u)|(d.mips>1?1u:0u);if(d.cube)s.u[Type]=1;if(d.alpha)s.u[Flags]|=1u<<25;}
}
