#pragma once
#include <juce_core/juce_core.h>
#include <juce_cryptography/juce_cryptography.h>
#include <array>
#include <atomic>
#include <cstdint>
#include <vector>
namespace pulse {
// WebSocket handshake SHA-1 (RFC 6455, not used for password hashing).
inline juce::String websocketAccept(const juce::String& key){
 auto raw=key+"258EAFA5-E914-47DA-95CA-C5AB0DC85B11";auto* bytes=(const uint8_t*)raw.toRawUTF8();size_t count=raw.getNumBytesAsUTF8();
 std::vector<uint8_t> data(bytes,bytes+count);data.push_back(0x80);while(data.size()%64!=56)data.push_back(0);
 uint64_t bits=(uint64_t)count*8;for(int i=7;i>=0;--i)data.push_back((uint8_t)(bits>>(8*i)));
 uint32_t h[]={0x67452301u,0xefcdab89u,0x98badcfeu,0x10325476u,0xc3d2e1f0u};
 auto rol=[](uint32_t x,int n){return (x<<n)|(x>>(32-n));};
 for(size_t off=0;off<data.size();off+=64){uint32_t w[80]{};for(int i=0;i<16;++i)for(int j=0;j<4;++j)w[i]=(w[i]<<8)|data[off+4*i+j];for(int i=16;i<80;++i)w[i]=rol(w[i-3]^w[i-8]^w[i-14]^w[i-16],1);
  uint32_t a=h[0],b=h[1],c=h[2],d=h[3],e=h[4];for(int i=0;i<80;++i){uint32_t f,k;if(i<20){f=(b&c)|(~b&d);k=0x5a827999;}else if(i<40){f=b^c^d;k=0x6ed9eba1;}else if(i<60){f=(b&c)|(b&d)|(c&d);k=0x8f1bbcdc;}else{f=b^c^d;k=0xca62c1d6;}uint32_t t=rol(a,5)+f+e+k+w[i];e=d;d=c;c=rol(b,30);b=a;a=t;}h[0]+=a;h[1]+=b;h[2]+=c;h[3]+=d;h[4]+=e;}
 uint8_t digest[20];for(int i=0;i<20;++i)digest[i]=(uint8_t)(h[i/4]>>(24-8*(i%4)));return juce::Base64::toBase64(digest,sizeof digest);
}
inline juce::String obsAuth(const juce::String& password,const juce::String& salt,const juce::String& challenge){
 auto hash=[](const juce::String& s){auto b=juce::SHA256(s.toRawUTF8(),s.getNumBytesAsUTF8()).getRawData();return juce::Base64::toBase64(b.getData(),b.getSize());};
 return hash(hash(password+salt)+challenge);
}
class ObsStatus final : private juce::Thread {
 juce::String password;int port=4455;double deadline=0;juce::StreamingSocket socket;
 bool readExact(void* dest,int size){auto* p=(char*)dest;while(size>0&&!threadShouldExit()&&juce::Time::getMillisecondCounterHiRes()<deadline){int ready=socket.waitUntilReady(true,100);if(ready<0)return false;if(!ready)continue;int n=socket.read(p,size,false);if(n<=0)return false;p+=n;size-=n;}return size==0;}
 bool writeAll(const void* src,int size){auto* p=(const char*)src;while(size>0&&!threadShouldExit()&&juce::Time::getMillisecondCounterHiRes()<deadline){int ready=socket.waitUntilReady(false,100);if(ready<0)return false;if(!ready)continue;int n=socket.write(p,size);if(n<=0)return false;p+=n;size-=n;}return size==0;}
 bool frame(int opcode,const void* data,int n){if(n<0||n>65535)return false;std::vector<uint8_t> b{(uint8_t)(0x80|opcode)};if(n<126)b.push_back((uint8_t)(0x80|n));else{b.push_back(0xfe);b.push_back((uint8_t)(n>>8));b.push_back((uint8_t)n);}uint8_t key[4];for(auto& k:key)k=(uint8_t)juce::Random::getSystemRandom().nextInt(256);for(auto k:key)b.push_back(k);auto* p=(const uint8_t*)data;for(int i=0;i<n;++i)b.push_back(p[i]^key[i%4]);return writeAll(b.data(),(int)b.size());}
 bool send(const juce::String& s){return frame(1,s.toRawUTF8(),(int)s.getNumBytesAsUTF8());}
 juce::var receive(){for(int attempt=0;attempt<12;++attempt){uint8_t h[2];if(!readExact(h,2)||(h[0]&0x70)||(h[1]&0x80)||!(h[0]&0x80))return {};int op=h[0]&15;uint64_t n=h[1]&127;if(n==126){uint8_t x[2];if(!readExact(x,2))return {};n=(x[0]<<8)|x[1];}else if(n==127){uint8_t x[8];if(!readExact(x,8))return {};n=0;for(auto v:x)n=(n<<8)|v;}if(n>65536)return {};juce::MemoryBlock b((size_t)n);if(n&&!readExact(b.getData(),(int)n))return {};if(op==8)return {};if(op==9){if(n>125||!frame(10,b.getData(),(int)n))return {};continue;}if(op==10)continue;if(op!=1)return {};return juce::JSON::parse(juce::String::fromUTF8((const char*)b.getData(),(int)n));}return {};}
 bool upgrade(){uint8_t random[16];for(auto& v:random)v=(uint8_t)juce::Random::getSystemRandom().nextInt(256);auto key=juce::Base64::toBase64(random,sizeof random);
  auto request="GET / HTTP/1.1\r\nHost: 127.0.0.1:"+juce::String(port)+"\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Key: "+key+"\r\nSec-WebSocket-Version: 13\r\nSec-WebSocket-Protocol: obswebsocket.json\r\n\r\n";
  if(!writeAll(request.toRawUTF8(),(int)request.getNumBytesAsUTF8()))return false;
  std::string response;char c;while(response.size()<8192){if(!readExact(&c,1))return false;response+=c;if(response.size()>=4&&response.compare(response.size()-4,4,"\r\n\r\n")==0)break;}auto lines=juce::StringArray::fromLines(juce::String::fromUTF8(response.data(),(int)response.size()));if(lines.isEmpty()||!lines[0].startsWith("HTTP/1.1 101"))return false;for(auto& line:lines){if(line.upToFirstOccurrenceOf(":",false,false).equalsIgnoreCase("Sec-WebSocket-Accept"))return line.fromFirstOccurrenceOf(":",false,false).trim()==websocketAccept(key);}return false;}
 int query(const juce::String& type,const juce::String& id){if(!send("{\"op\":6,\"d\":{\"requestType\":\""+type+"\",\"requestId\":\""+id+"\"}}"))return -1;for(int i=0;i<8;++i){auto v=receive();if(v.isVoid())return -1;if((int)v.getProperty("op",-1)!=7)continue;auto d=v.getProperty("d",{});if(d.getProperty("requestId",{}).toString()!=id)continue;auto s=d.getProperty("requestStatus",{});if(!(bool)s.getProperty("result",false))return -1;auto output=d.getProperty("responseData",{}).getProperty("outputActive",{});if(!output.isBool())return -1;return (bool)output?1:0;}return -1;}
 void run() override {
  deadline=juce::Time::getMillisecondCounterHiRes()+4000;
  auto check=[&]()->bool{if(!socket.connect("127.0.0.1",port,800)||!upgrade())return false;auto hello=receive();if((int)hello.getProperty("op",-1)!=0)return false;
   auto d=hello.getProperty("d",{});if((int)d.getProperty("rpcVersion",0)<1)return false;auto a=d.getProperty("authentication",{});juce::String auth;
   if(a.isObject()){auto salt=a.getProperty("salt",{}).toString(),challenge=a.getProperty("challenge",{}).toString();if(salt.isEmpty()||challenge.isEmpty()||password.isEmpty())return false;auth=",\"authentication\":\""+obsAuth(password,salt,challenge)+"\"";}
   if(!send("{\"op\":1,\"d\":{\"rpcVersion\":1,\"eventSubscriptions\":0"+auth+"}}")||(int)receive().getProperty("op",-1)!=2)return false;
   int s=query("GetStreamStatus","pulse-stream"),r=query("GetRecordStatus","pulse-record");if(s<0||r<0)return false;streaming=s;recording=r;checkedAt=juce::Time::getCurrentTime().toMilliseconds();return true;};
  bool ok=check();socket.close();password.clear();state=ok?2:3;
 }
public:
 std::atomic<int> state{0},streaming{-1},recording{-1};std::atomic<juce::int64> checkedAt{0};
 ObsStatus():Thread("OBS status"){}
 ~ObsStatus() override{signalThreadShouldExit();socket.close();stopThread(5000);}
 bool check(int p,const juce::String& pw){if(isThreadRunning()||p<1||p>65535)return false;port=p;password=pw;streaming=-1;recording=-1;state=1;startThread();return true;}
};
}
