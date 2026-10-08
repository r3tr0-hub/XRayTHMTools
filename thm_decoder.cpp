#include "thm_decoder.h"
#include <stdexcept>
#include <algorithm>
namespace {
uint32_t u32(const uint8_t* p){return uint32_t(p[0])|uint32_t(p[1])<<8|uint32_t(p[2])<<16|uint32_t(p[3])<<24;}
struct LZH {
 static constexpr int N=4096,F=60,NC=314,T=627,R=626;
 unsigned freq[T+1]{}; int parent[T+NC]{},son[T]{};
 const uint8_t* data; size_t size,bit=0;
 LZH(const uint8_t* p,size_t n):data(p),size(n){
 for(int i=0;i<NC;i++){freq[i]=1;son[i]=i+T;parent[i+T]=i;}
 for(int i=NC,j=0;i<=R;i++,j+=2){freq[i]=freq[j]+freq[j+1];son[i]=j;parent[j]=parent[j+1]=i;}
 freq[T]=65535;parent[R]=0;
 }
 int get(){if(bit>=size*8)throw std::runtime_error("Truncated LZH stream");int v=(data[bit/8]>>(7-bit%8))&1;bit++;return v;}
 void rebuild(){
 int j=0;for(int i=0;i<T;i++)if(son[i]>=T){freq[j]=(freq[i]+1)/2;son[j++]=son[i];}
 for(int i=NC,k=0;i<T;i++,k+=2){unsigned f=freq[k]+freq[k+1];int at=i-1;while(at>=0&&f<freq[at])at--;at++;
 for(int z=i;z>at;z--){freq[z]=freq[z-1];son[z]=son[z-1];}freq[at]=f;son[at]=k;}
 for(int i=0;i<T;i++){int k=son[i];if(k>=T)parent[k]=i;else parent[k]=parent[k+1]=i;}
 }
 void update(int c){if(freq[R]==32768)rebuild();c=parent[c+T];do{
 unsigned f=++freq[c];int l=c+1;if(f>freq[l]){while(f>freq[++l]){}--l;freq[c]=freq[l];freq[l]=f;
 int a=son[c];parent[a]=l;if(a<T)parent[a+1]=l;int b=son[l];son[l]=a;parent[b]=c;if(b<T)parent[b+1]=c;son[c]=b;c=l;}
 c=parent[c];}while(c!=0);}
 int symbol(){int c=son[R];while(c<T)c=son[c+get()];c-=T;update(c);return c;}
 int position(){// Canonical upper-six-bit position codes of classic LZHUF.
 static const int counts[]={0,0,0,1,3,8,12,24,16};
 int code=0,first=0,index=0;
 for(int len=1;len<=8;len++){code=(code<<1)|get();if(code-first<counts[len]){int upper=index+code-first;int lower=0;for(int i=0;i<6;i++)lower=(lower<<1)|get();return (upper<<6)|lower;}index+=counts[len];first=(first+counts[len])<<1;}
 throw std::runtime_error("Invalid LZH position");}
 std::vector<uint8_t> run(){std::vector<uint8_t> out;out.reserve(65536);uint8_t ring[N];std::fill(ring,ring+N,uint8_t{32});int r=N-F;
 while(out.size()<65536){int c=symbol();if(c<256){out.push_back(uint8_t(c));ring[r]=uint8_t(c);r=(r+1)&(N-1);}else{
 int pos=(r-position()-1)&(N-1),len=c-255+2;if(out.size()+len>65536)throw std::runtime_error("LZH output overrun");
 for(int k=0;k<len;k++){uint8_t v=ring[(pos+k)&(N-1)];out.push_back(v);ring[r]=v;r=(r+1)&(N-1);}}}return out;}
};
}
std::vector<uint8_t> decode_thm(const std::vector<uint8_t>& f){
 if(f.size()>4*1024*1024)throw std::runtime_error("THM too large");
 const uint8_t* image=nullptr;size_t length=0;bool compressed=false,version=false;
 for(size_t o=0;o<f.size();){if(f.size()-o<8)throw std::runtime_error("Truncated chunk");uint32_t id=u32(&f[o]),n=u32(&f[o+4]);o+=8;if(n>f.size()-o)throw std::runtime_error("Invalid chunk size");
 if((id&0x7fffffff)==0x810){version=n==2&&f[o]==0x12&&f[o+1]==0;}
 if((id&0x7fffffff)==0x811){if(image)throw std::runtime_error("Duplicate image");image=&f[o];length=n;compressed=(id>>31)!=0;}o+=n;}
 if(!version||!image)throw std::runtime_error("Unsupported object THM");
 if(!compressed){if(length!=65536)throw std::runtime_error("Invalid image size");return {image,image+length};}
 if(length<5||u32(image)!=65536)throw std::runtime_error("Invalid unpacked size");
 return LZH(image+4,length-4).run();
}
