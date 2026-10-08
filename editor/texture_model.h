#pragma once
#include "thm_document.h"
#include <array>
#include <string>
#include <vector>
namespace texture {
enum UIntField {Format,Type,Material,BumpMode,MipFilter,Flags,BorderColor,FadeColor,FadeAmount,Width,Height,FadeDelay,UIntCount};
enum FloatField {DetailScale,MaterialWeight,BumpHeight,FloatCount};
struct Settings {
 std::array<uint32_t,UIntCount> u{{0,0,1,1,0,0,0,0,0,0,0,0}};
 std::array<float,FloatCount> f{{1.0f,0.0f,0.05f}};
 std::array<std::string,3> names;
 bool soc=false;
};
struct Document {std::vector<uint8_t> original;ThmDocument container;Settings initial,current;};
Document load(const std::vector<uint8_t>& file);
Document create();
std::vector<uint8_t> serialize(const Document& doc);
bool equal(const Settings&,const Settings&);
uint32_t parse_uint(const std::string& text);
float parse_float(std::string text);
std::string float_text(float);
std::string enum_name(UIntField,uint32_t);
std::vector<std::string> enum_names(UIntField);
struct Flag {unsigned bit;const char* name;};
const std::array<Flag,12>& flag_list();
void apply_type_defaults(Settings&);
struct Dds {uint32_t width,height,mips;int format=-1;bool cube=false,alpha=false;std::string warning;};
Dds read_dds_header(const std::vector<uint8_t>& header);
void import_dds(Settings&,const Dds&);
}
