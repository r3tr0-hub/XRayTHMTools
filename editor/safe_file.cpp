#define NOMINMAX
#include <windows.h>
#include "safe_file.h"
#include <stdexcept>
#include <algorithm>
#include <cwchar>
namespace {
struct Handle {HANDLE h=INVALID_HANDLE_VALUE;explicit Handle(HANDLE v):h(v){}~Handle(){if(h!=INVALID_HANDLE_VALUE)CloseHandle(h);}Handle(const Handle&)=delete;};
std::runtime_error error(const char* msg){return std::runtime_error(std::string(msg)+" (Windows error "+std::to_string(GetLastError())+")");}
std::vector<uint8_t> fromHandle(HANDLE h,size_t limit,bool prefix){LARGE_INTEGER n{};if(!GetFileSizeEx(h,&n))throw error("Cannot read size");if(n.QuadPart<0||(!prefix&&uint64_t(n.QuadPart)>limit))throw std::runtime_error("File exceeds size limit");size_t count=static_cast<size_t>(std::min<uint64_t>(uint64_t(n.QuadPart),limit));std::vector<uint8_t>b(count);DWORD got=0;if(count&&(!ReadFile(h,b.data(),DWORD(count),&got,nullptr)||got!=count))throw error("Cannot read file");return b;}
}
std::vector<uint8_t> read_bytes(const std::wstring& p,size_t limit){Handle h(CreateFileW(p.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr));if(h.h==INVALID_HANDLE_VALUE)throw error("Cannot open file");return fromHandle(h.h,limit,false);}
std::vector<uint8_t> read_header(const std::wstring& p,size_t count){Handle h(CreateFileW(p.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr));if(h.h==INVALID_HANDLE_VALUE)throw error("Cannot open DDS");return fromHandle(h.h,count,true);}
std::wstring save_atomic(const std::wstring& path,const std::vector<uint8_t>& data,const std::vector<uint8_t>* expected){
 Handle old(CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr));
 if(expected){if(old.h==INVALID_HANDLE_VALUE)throw error("Original file unavailable");if(fromHandle(old.h,4*1024*1024,false)!=*expected)throw std::runtime_error("File changed on disk. Reopen it or use Save As; nothing was overwritten.");}
 else {if(old.h!=INVALID_HANDLE_VALUE)throw std::runtime_error("Destination already exists");if(GetLastError()!=ERROR_FILE_NOT_FOUND)throw error("Destination not available");}
 std::wstring temp,backup;bool created=false;
 for(int i=0;i<100;i++){temp=path+L".tmp-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(GetTickCount64())+L"-"+std::to_wstring(i);Handle h(CreateFileW(temp.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr));if(h.h==INVALID_HANDLE_VALUE){if(GetLastError()==ERROR_FILE_EXISTS)continue;throw error("Cannot create temporary file");}DWORD done=0;bool ok=WriteFile(h.h,data.data(),DWORD(data.size()),&done,nullptr)&&done==data.size()&&FlushFileBuffers(h.h);if(!ok){auto e=error("Cannot flush temporary file");CloseHandle(h.h);h.h=INVALID_HANDLE_VALUE;DeleteFileW(temp.c_str());throw e;}created=true;break;}
 if(!created)throw std::runtime_error("No temporary file created");
 bool ok=false;
 if(expected){SYSTEMTIME t{};GetLocalTime(&t);wchar_t stamp[96]{};swprintf_s(stamp,L".bak-%04u%02u%02u-%02u%02u%02u-%llu",t.wYear,t.wMonth,t.wDay,t.wHour,t.wMinute,t.wSecond,static_cast<unsigned long long>(GetTickCount64()));backup=path+stamp;
 if(GetFileAttributesW(backup.c_str())!=INVALID_FILE_ATTRIBUTES){DeleteFileW(temp.c_str());throw std::runtime_error("Backup name collision. Retry save.");}
 ok=ReplaceFileW(path.c_str(),temp.c_str(),backup.c_str(),0,nullptr,nullptr)!=FALSE;
 }else ok=MoveFileExW(temp.c_str(),path.c_str(),MOVEFILE_WRITE_THROUGH)!=FALSE;
 if(!ok){auto e=error("Atomic save failed; inspect original and backup before retrying");DeleteFileW(temp.c_str());throw e;}return backup;
}
