#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include <commctrl.h>
#include <algorithm>
#include <memory>
#include <string>
#include <vector>
#include <cstring>
#include <cwchar>
#include <stdexcept>
#include "thm_document.h"
namespace {
HINSTANCE module=nullptr;
constexpr wchar_t className[]=L"XRayTHMTools.Lister.v04";
constexpr wchar_t pictureClass[]=L"XRayTHMTools.Preview.v04";
struct State {ThmDocument doc;std::wstring path;HWND edit=nullptr,picture=nullptr;HWND buttons[6]{};HFONT font=nullptr;int zoom=0;};
struct Creation {State* state;bool attached=false;};
std::wstring wide(const std::string& s,UINT cp=1251){if(s.empty())return {};int n=MultiByteToWideChar(cp,0,s.data(),int(s.size()),nullptr,0);if(n<=0)return L"[encoding error]";std::wstring out(n,0);MultiByteToWideChar(cp,0,s.data(),int(s.size()),out.data(),n);return out;}
std::vector<uint8_t> readFile(const wchar_t* path){
 HANDLE h=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
 if(h==INVALID_HANDLE_VALUE)throw std::runtime_error("Cannot open THM");
 struct Close {HANDLE h;~Close(){CloseHandle(h);}} close{h};
 LARGE_INTEGER n{};if(!GetFileSizeEx(h,&n)||n.QuadPart<1||n.QuadPart>4*1024*1024)throw std::runtime_error("File must be 1 byte to 4 MiB");
 std::vector<uint8_t> b(static_cast<size_t>(n.QuadPart));DWORD got=0;if(!ReadFile(h,b.data(),DWORD(b.size()),&got,nullptr)||got!=b.size())throw std::runtime_error("Read failed");return b;
}
bool isThm(const wchar_t* path){if(!path)return false;const wchar_t* ext=wcsrchr(path,L'.');return ext&&(_wcsicmp(ext,L".thm")==0||_wcsicmp(ext,L".object")==0||_wcsicmp(ext,L".dds")==0);}
std::wstring metadataPath(const wchar_t* path){std::wstring p=path;size_t dot=p.find_last_of(L'.');if(dot!=std::wstring::npos)p.replace(dot,std::wstring::npos,L".thm");return p;}
void load(State& s,const wchar_t* path){s.path=path;try{auto meta=metadataPath(path);s.doc=inspect_thm(readFile(meta.c_str()));}catch(const std::exception& e){s.doc={};s.doc.report=std::string("Cannot read this THM\r\n\r\n")+e.what()+"\r\n\r\nFile was not modified.";}}
void copy(HWND owner,const std::wstring& text){if(!OpenClipboard(owner))return;size_t bytes=(text.size()+1)*sizeof(wchar_t);HGLOBAL mem=GlobalAlloc(GMEM_MOVEABLE,bytes);if(mem){void* p=GlobalLock(mem);if(p){std::memcpy(p,text.c_str(),bytes);GlobalUnlock(mem);EmptyClipboard();if(!SetClipboardData(CF_UNICODETEXT,mem))GlobalFree(mem);}else GlobalFree(mem);}CloseClipboard();}
void layout(HWND window,State& s){RECT r{};GetClientRect(window,&r);int width=r.right,height=r.bottom;
 int columns=std::clamp((width-16)/84,1,6);
 for(int i=0;i<6;i++)MoveWindow(s.buttons[i],8+(i%columns)*84,6+(i/columns)*32,80,26,TRUE);
 bool image=!s.doc.preview.empty();ShowWindow(s.picture,image?SW_SHOW:SW_HIDE);
 int top=12+((6+columns-1)/columns)*32;
 if(image&&width>=600){int pw=std::min(360,width/2);MoveWindow(s.picture,8,top,pw-16,std::max(0,height-top-8),TRUE);MoveWindow(s.edit,pw,top,std::max(0,width-pw-8),std::max(0,height-top-8),TRUE);}
 else {int ph=image?std::min(280,std::max(0,(height-top)/2)):0;MoveWindow(s.picture,8,top,std::max(0,width-16),ph,TRUE);MoveWindow(s.edit,8,top+ph,std::max(0,width-16),std::max(0,height-top-ph-8),TRUE);}
 for(int i=0;i<3;i++)EnableWindow(s.buttons[i],image);
 EnableWindow(s.buttons[4],s.doc.type==1&&s.doc.version==18);
}
LRESULT CALLBACK pictureProc(HWND w,UINT msg,WPARAM wp,LPARAM lp){
 if(msg==WM_ERASEBKGND)return 1;
 if(msg==WM_PAINT){PAINTSTRUCT ps{};HDC dc=BeginPaint(w,&ps);RECT r{};GetClientRect(w,&r);FillRect(dc,&r,GetSysColorBrush(COLOR_BTNFACE));auto s=reinterpret_cast<State*>(GetWindowLongPtrW(GetParent(w),GWLP_USERDATA));
 if(s&&!s->doc.preview.empty()){int maxSide=std::max(0,static_cast<int>(std::min(r.right,r.bottom))-16);int side=std::min(s->zoom?128*s->zoom:512,maxSide);
 if(side>0){BITMAPINFO bi{};bi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);bi.bmiHeader.biWidth=128;bi.bmiHeader.biHeight=128;bi.bmiHeader.biPlanes=1;bi.bmiHeader.biBitCount=32;bi.bmiHeader.biCompression=BI_RGB;SetStretchBltMode(dc,COLORONCOLOR);StretchDIBits(dc,(r.right-side)/2,(r.bottom-side)/2,side,side,0,0,128,128,s->doc.preview.data(),&bi,DIB_RGB_COLORS,SRCCOPY);}}
 EndPaint(w,&ps);return 0;}return DefWindowProcW(w,msg,wp,lp);
}
int next(HWND window,const wchar_t* path);
void editTexture(HWND w,State& s){
 if(s.doc.type!=1||s.doc.version!=18)return;
 wchar_t filename[32768]{};if(!GetModuleFileNameW(module,filename,32768))return;
 std::wstring exe=filename;exe.resize(exe.find_last_of(L"\\/")+1);
#ifdef _WIN64
 exe+=L"XRayTHMEditor.exe";
#else
 exe+=L"XRayTHMEditor32.exe";
#endif
 std::wstring args=L"\""+metadataPath(s.path.c_str())+L"\"";
 if(reinterpret_cast<INT_PTR>(ShellExecuteW(w,L"open",exe.c_str(),args.c_str(),nullptr,SW_SHOWNORMAL))<=32)
 MessageBoxW(w,L"Cannot start XRayTHMEditor. Install the complete plugin ZIP, including the editor EXE.",L"X-Ray THM Tools",MB_ICONERROR);
}
LRESULT CALLBACK editKeys(HWND w,UINT msg,WPARAM wp,LPARAM lp,UINT_PTR id,DWORD_PTR){
 // Let Total Commander handle Esc (both standalone Lister and Quick View).
 // Route through the plugin window so the host's subclass can see the key.
 if((msg==WM_KEYDOWN||msg==WM_KEYUP)&&wp==VK_ESCAPE){PostMessageW(GetParent(w),msg,wp,lp);return 0;}
 if(msg==WM_CHAR&&wp==VK_ESCAPE)return 0;
 if(msg==WM_KEYDOWN&&wp==VK_F4){SendMessageW(GetParent(w),WM_COMMAND,104,0);return 0;}
 if(msg==WM_NCDESTROY)RemoveWindowSubclass(w,editKeys,id);
 return DefSubclassProc(w,msg,wp,lp);
}
LRESULT windowProcImpl(HWND w,UINT msg,WPARAM wp,LPARAM lp){
 auto s=reinterpret_cast<State*>(GetWindowLongPtrW(w,GWLP_USERDATA));
 if(msg==WM_NCCREATE){auto cs=reinterpret_cast<CREATESTRUCTW*>(lp);auto context=static_cast<Creation*>(cs->lpCreateParams);s=context->state;context->attached=true;SetWindowLongPtrW(w,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(s));return TRUE;}
 switch(msg){
 case WM_CREATE:{if(!s)return -1;s->font=CreateFontW(-15,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,FIXED_PITCH,L"Consolas");
 const wchar_t* labels[]={L"Fit",L"100%",L"200%",L"Copy report",L"Edit (F4)",L"Reload"};
 for(int i=0;i<6;i++)s->buttons[i]=CreateWindowExW(0,L"BUTTON",labels[i],WS_CHILD|WS_VISIBLE|WS_TABSTOP,0,0,0,0,w,reinterpret_cast<HMENU>(static_cast<INT_PTR>(100+i)),module,nullptr);
 s->picture=CreateWindowExW(0,pictureClass,L"",WS_CHILD|WS_VISIBLE,0,0,0,0,w,nullptr,module,nullptr);
 s->edit=CreateWindowExW(WS_EX_CLIENTEDGE,L"EDIT",L"",WS_CHILD|WS_VISIBLE|WS_VSCROLL|WS_HSCROLL|ES_MULTILINE|ES_READONLY|ES_AUTOVSCROLL|ES_AUTOHSCROLL,0,0,0,0,w,nullptr,module,nullptr);
 if(!s->edit||!s->picture||!s->buttons[5])return -1;
 SetWindowSubclass(s->edit,editKeys,1,0);
 SetWindowSubclass(s->picture,editKeys,1,0);
 for(HWND button:s->buttons)SetWindowSubclass(button,editKeys,1,0);
 SendMessageW(s->edit,WM_SETFONT,reinterpret_cast<WPARAM>(s->font),TRUE);SendMessageW(s->edit,EM_SETLIMITTEXT,1024*1024,0);
 std::wstring report=L"File: "+s->path+L"\r\n\r\n"+wide(s->doc.report);SetWindowTextW(s->edit,report.c_str());layout(w,*s);return 0;}
 case WM_KEYDOWN:
 case WM_KEYUP:
  if(wp==VK_ESCAPE){PostMessageW(GetParent(w),msg,wp,lp);return 0;}
  if(msg==WM_KEYDOWN&&s&&wp==VK_F4){editTexture(w,*s);return 0;}break;
 case WM_CHAR:if(wp==VK_ESCAPE)return 0;break;
 case WM_SIZE:if(s)layout(w,*s);return 0;
 case WM_SETFOCUS:if(s)SetFocus(s->edit);return 0;
 case WM_COMMAND:if(s&&HIWORD(wp)==BN_CLICKED){int id=LOWORD(wp);if(id>=100&&id<=102){s->zoom=id==100?0:id==101?1:2;InvalidateRect(s->picture,nullptr,TRUE);}else if(id==103)copy(w,L"File: "+s->path+L"\r\n\r\n"+wide(s->doc.report));else if(id==104)editTexture(w,*s);else if(id==105){auto path=s->path;next(w,path.c_str());}return 0;}break;
 case WM_NCDESTROY:if(s){if(s->font)DeleteObject(s->font);SetWindowLongPtrW(w,GWLP_USERDATA,0);delete s;}break;
 }
 return DefWindowProcW(w,msg,wp,lp);
}
LRESULT CALLBACK windowProc(HWND w,UINT msg,WPARAM wp,LPARAM lp){try{return windowProcImpl(w,msg,wp,lp);}catch(...){return msg==WM_CREATE?-1:DefWindowProcW(w,msg,wp,lp);}}
bool registerClasses(){WNDCLASSEXW wc{sizeof(wc)};wc.hInstance=module;wc.hCursor=LoadCursorW(nullptr,IDC_ARROW);wc.hbrBackground=GetSysColorBrush(COLOR_BTNFACE);wc.lpszClassName=className;wc.lpfnWndProc=windowProc;
 if(!RegisterClassExW(&wc)&&GetLastError()!=ERROR_CLASS_ALREADY_EXISTS)return false;
 wc.lpszClassName=pictureClass;wc.lpfnWndProc=pictureProc;return RegisterClassExW(&wc)||GetLastError()==ERROR_CLASS_ALREADY_EXISTS;
}
HWND open(HWND parent,const wchar_t* path){if(!isThm(path)||!registerClasses())return nullptr;
 auto state=std::make_unique<State>();load(*state,path);RECT r{};GetClientRect(parent,&r);
 // WM_NCDESTROY owns state once WM_NCCREATE has run; on pre-creation failure reclaim it.
 State* raw=state.release();Creation context{raw};HWND w=CreateWindowExW(0,className,L"X-Ray THM Lister",WS_CHILD|WS_VISIBLE|WS_CLIPCHILDREN,0,0,r.right,r.bottom,parent,nullptr,module,&context);
 if(!w&&!context.attached)delete raw;
 return w;
}
int next(HWND window,const wchar_t* path){if(!isThm(path))return 1;auto s=reinterpret_cast<State*>(GetWindowLongPtrW(window,GWLP_USERDATA));if(!s)return 1;load(*s,path);s->zoom=0;auto report=L"File: "+s->path+L"\r\n\r\n"+wide(s->doc.report);SetWindowTextW(s->edit,report.c_str());layout(window,*s);InvalidateRect(s->picture,nullptr,TRUE);return 0;}
}
extern "C" HWND __stdcall ListLoadW(HWND p,wchar_t* file,int){try{return open(p,file);}catch(...){return nullptr;}}
extern "C" HWND __stdcall ListLoad(HWND p,char* file,int){try{if(!file)return nullptr;auto path=wide(file,CP_ACP);return open(p,path.c_str());}catch(...){return nullptr;}}
extern "C" int __stdcall ListLoadNextW(HWND,HWND w,wchar_t* file,int){try{return next(w,file);}catch(...){return 1;}}
extern "C" int __stdcall ListLoadNext(HWND,HWND w,char* file,int){try{if(!file)return 1;auto path=wide(file,CP_ACP);return next(w,path.c_str());}catch(...){return 1;}}
extern "C" void __stdcall ListCloseWindow(HWND w){DestroyWindow(w);}
extern "C" void __stdcall ListGetDetectString(char* out,int n){const char* text="EXT=\"THM\" | EXT=\"OBJECT\"";if(out&&n>0){size_t size=std::min<size_t>(std::strlen(text),size_t(n-1));std::memcpy(out,text,size);out[size]=0;}}
BOOL WINAPI DllMain(HINSTANCE h,DWORD reason,LPVOID){if(reason==DLL_PROCESS_ATTACH){module=h;DisableThreadLibraryCalls(h);}else if(reason==DLL_PROCESS_DETACH){UnregisterClassW(pictureClass,module);UnregisterClassW(className,module);}return TRUE;}
