#pragma once
#include <string>
#include <vector>
#include <cstdint>
std::vector<uint8_t> read_bytes(const std::wstring&,size_t limit=4*1024*1024);
std::vector<uint8_t> read_header(const std::wstring&,size_t count=148);
// existing files require an exact expected snapshot; nullptr means new file only.
std::wstring save_atomic(const std::wstring&,const std::vector<uint8_t>&,const std::vector<uint8_t>* expected);
