#pragma once
#include <cstdint>
#include <string>
#include <vector>
struct ThmChunk {uint32_t id;bool compressed;std::vector<uint8_t> bytes;};
struct ThmDocument {
 uint16_t version=0;uint32_t type=0xffffffff;
 std::vector<ThmChunk> chunks;
 std::vector<uint8_t> preview; // original bottom-up BGRA; display flips rows
 std::string report;
};
ThmDocument inspect_thm(const std::vector<uint8_t>& file);
