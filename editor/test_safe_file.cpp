#define NOMINMAX
#include <windows.h>
#include "safe_file.h"
#include <filesystem>
#include <iostream>
#include <stdexcept>
namespace fs=std::filesystem;
static void require(bool b,const char* msg){if(!b)throw std::runtime_error(msg);}
int main(){fs::path dir;try{
 wchar_t temp[MAX_PATH]{};require(GetTempPathW(MAX_PATH,temp)!=0,"temp path");dir=fs::path(temp)/(L"XRayTHM-save-test-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(GetTickCount64()));require(fs::create_directory(dir),"create test folder");auto path=(dir/L"texture.thm").wstring();std::vector<uint8_t> a{1,2,3},b{4,5,6,7},c{8,9};
 require(save_atomic(path,a,nullptr).empty(),"new file backup");require(read_bytes(path)==a,"new data");bool rejected=false;try{save_atomic(path,b,nullptr);}catch(...){rejected=true;}require(rejected&&read_bytes(path)==a,"new must not overwrite");
 auto backup=save_atomic(path,b,&a);require(read_bytes(path)==b&&read_bytes(backup)==a,"replace and backup");rejected=false;try{save_atomic(path,c,&a);}catch(...){rejected=true;}require(rejected&&read_bytes(path)==b,"stale snapshot overwrite");
 HANDLE h=CreateFileW(path.c_str(),GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,0,nullptr);require(h!=INVALID_HANDLE_VALUE,"writer handle");rejected=false;try{save_atomic(path,c,&b);}catch(...){rejected=true;}CloseHandle(h);require(rejected&&read_bytes(path)==b,"active writer protection");
 fs::remove_all(dir);std::cout<<"Atomic save, backups, conflicts and writer lock passed\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';if(!dir.empty()){std::error_code ec;fs::remove_all(dir,ec);}return 1;}}
