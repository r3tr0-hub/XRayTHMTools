#include "thm_document.h"
#include "thm_decoder.h"
#include "editor/texture_model.h"
#include <sstream>
#include <iomanip>
#include <cstring>
#include <cmath>
#include <stdexcept>
#include <algorithm>
namespace {
uint32_t u32(const std::vector<uint8_t>& b,size_t o){if(o>b.size()||b.size()-o<4)throw std::runtime_error("Truncated u32");return uint32_t(b[o])|uint32_t(b[o+1])<<8|uint32_t(b[o+2])<<16|uint32_t(b[o+3])<<24;}
float f32(const std::vector<uint8_t>& b,size_t o){uint32_t n=u32(b,o);float f;static_assert(sizeof(f)==4,"32-bit float required");std::memcpy(&f,&n,4);return f;}
std::string hex(uint32_t v){std::ostringstream s;s<<"0x"<<std::hex<<std::uppercase<<std::setw(8)<<std::setfill('0')<<v;return s.str();}
std::string number(float f){if(!std::isfinite(f))return "invalid (non-finite)";std::ostringstream s;s<<std::setprecision(8)<<f;return s.str();}
std::string stringz(const std::vector<uint8_t>& b,size_t& o){if(o>b.size())throw std::runtime_error("Bad string offset");size_t end=o;while(end<b.size()&&b[end])end++;if(end==b.size())throw std::runtime_error("Unterminated string");std::string out;for(size_t i=o;i<end;i++){if(b[i]>=32)out.push_back(char(b[i]));else out+='?';}o=end+1;return out;}
const char* title(uint32_t n){switch(n){case 0x810:return "Version";case 0x811:return "Thumbnail";case 0x812:return "Texture parameters";case 0x813:return "THM type";case 0x814:return "Texture type";case 0x815:return "Detail texture";case 0x816:return "Material / object statistics";case 0x817:return "Bump";case 0x818:return "External normal map";case 0x819:return "Fade delay";default:return "Unknown extension";}}
}
ThmDocument inspect_thm(const std::vector<uint8_t>& file){
 if(file.empty()||file.size()>4*1024*1024)throw std::runtime_error("Empty or oversized THM (limit 4 MiB)");
 ThmDocument d;bool version=false,type=false;
 for(size_t o=0;o<file.size();){if(file.size()-o<8)throw std::runtime_error("Truncated chunk header");uint32_t id=u32(file,o),size=u32(file,o+4);o+=8;if(size>file.size()-o)throw std::runtime_error("Chunk exceeds file size");if(d.chunks.size()>=1024)throw std::runtime_error("Too many chunks");
 ThmChunk c{id&0x7fffffff,(id&0x80000000)!=0,{file.begin()+o,file.begin()+o+size}};
 if(std::any_of(d.chunks.begin(),d.chunks.end(),[&](const ThmChunk& v){return v.id==c.id;}))throw std::runtime_error("Duplicate chunk");
 if(c.id==0x810){if(c.compressed||size!=2)throw std::runtime_error("Invalid version chunk");d.version=uint16_t(c.bytes[0]|uint16_t(c.bytes[1])<<8);version=true;}
 if(c.id==0x813){if(c.compressed||(size!=4&&size!=5))throw std::runtime_error("Invalid type chunk");d.type=u32(c.bytes,0);type=true;}
 d.chunks.push_back(std::move(c));o+=size;}
 if(!version||!type)throw std::runtime_error("Not a recognized X-Ray THM container (version/type missing)");
 std::ostringstream s;s<<"X-Ray THM Lister 0.4.0-beta\r\nREAD ONLY\r\n\r\n";
 s<<"File size: "<<file.size()<<" bytes\r\nVersion: "<<hex(d.version)<<"\r\nTHM type: "<<d.type<<" ("<<(d.type==0?"Object":d.type==1?"Texture":"Other")<<")\r\n";
 bool soc=false;for(const auto& c:d.chunks)if(c.id==0x813&&c.bytes.size()==5)soc=c.bytes[4]!=0;
 bool schema=d.version==0x12&&d.type==1;
 if(d.version!=0x12)s<<"Unknown version: only raw chunk information is shown.\r\n";
 s<<"\r\n";
 bool haveImage=false;
 for(const auto& c:d.chunks){
 s<<"["<<hex(c.id)<<"] "<<title(c.id)<<" | "<<c.bytes.size()<<" bytes"<<(c.compressed?" | compressed":"")<<"\r\n";
 if(c.id==0x811){haveImage=true;if(d.version==0x12&&(d.type==0||d.type==1)){try{d.preview=decode_thm(file);s<<"  Embedded thumbnail: 128 x 128 BGRA\r\n";}catch(const std::exception& e){s<<"  Thumbnail cannot be decoded: "<<e.what()<<"\r\n";}}}
 if(schema&&!c.compressed){try{
 const auto& b=c.bytes;size_t o=0;
 switch(c.id){
 case 0x812:{if(b.size()!=32)throw std::runtime_error("Expected classic 32-byte layout; displaying raw data instead");
 const char* names[]={soc?"Texture type ID (SoC swapped)":"Texture format ID","Flags","Border color","Fade color","Fade amount","Mip filter ID","Width","Height"};
 for(size_t i=0;i<8;i++){auto v=u32(b,i*4);s<<"  "<<names[i]<<": "<<((i>=1&&i<=3)?hex(v):std::to_string(v))<<"\r\n";}
 auto flags=u32(b,4);s<<"  Set flag bits:";for(int i=0;i<32;i++)if(flags&(uint32_t(1)<<i))s<<' '<<i;s<<"\r\n";for(const auto& flag:texture::flag_list())if(flags&(1u<<flag.bit))s<<"    "<<flag.name<<"\r\n";s<<"  Format/type: "<<texture::enum_name(soc?texture::Type:texture::Format,u32(b,0))<<"\r\n  Mip filter: "<<texture::enum_name(texture::MipFilter,u32(b,20))<<"\r\n";break;}
 case 0x814:if(b.size()!=4)throw std::runtime_error("Expected 4 bytes");s<<(soc?"  Texture format ID (SoC swapped): ":"  Texture type ID: ")<<u32(b,0)<<" ("<<texture::enum_name(soc?texture::Format:texture::Type,u32(b,0))<<")\r\n";break;
 case 0x815:{auto name=stringz(b,o);if(b.size()-o!=4)throw std::runtime_error("Unexpected detail layout");s<<"  Detail name: "<<(name.empty()?"(empty)":name)<<"\r\n  Detail scale: "<<number(f32(b,o))<<"\r\n";break;}
 case 0x816:if(b.size()!=8)throw std::runtime_error("Unexpected material layout");s<<"  Material ID: "<<u32(b,0)<<" ("<<texture::enum_name(texture::Material,u32(b,0))<<")\r\n  Material weight: "<<number(f32(b,4))<<"\r\n";break;
 case 0x817:{if(b.size()<9)throw std::runtime_error("Unexpected bump layout");o=8;auto name=stringz(b,o);if(o!=b.size())throw std::runtime_error("Trailing bump extension");s<<"  Bump height: "<<number(f32(b,0))<<"\r\n  Bump mode ID: "<<u32(b,4)<<" ("<<texture::enum_name(texture::BumpMode,u32(b,4))<<")\r\n  Bump name: "<<(name.empty()?"(empty)":name)<<"\r\n";break;}
 case 0x818:{auto name=stringz(b,o);if(o!=b.size())throw std::runtime_error("Trailing normal-map extension");s<<"  External normal map: "<<(name.empty()?"(empty)":name)<<"\r\n";break;}
 case 0x819:if(b.size()!=1)throw std::runtime_error("Expected 1 byte");s<<"  Fade delay: "<<unsigned(b[0])<<"\r\n";break;
 default:break;
 }
 }catch(const std::exception& e){s<<"  Layout warning: "<<e.what()<<"\r\n";}}
 if(c.id!=0x811){s<<"  Raw:";for(size_t i=0;i<std::min<size_t>(c.bytes.size(),128);i++)s<<' '<<std::hex<<std::setw(2)<<std::setfill('0')<<unsigned(c.bytes[i]);s<<std::dec<<std::setfill(' ');if(c.bytes.size()>128)s<<" ...";s<<"\r\n";}
 s<<"\r\n";
 }
 if(!haveImage)s<<"No embedded thumbnail. A texture settings THM does not need an image chunk.\r\n";
 s<<"\r\nTexture field labels use the classic v0x0012 layout. Modified SDK layouts may differ; raw values are retained.\r\n";
 d.report=s.str();return d;
}
