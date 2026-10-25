#define WIN32_LEAN_AND_MEAN
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0a00
#endif
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "status.h"

#define EXPORT __declspec(dllexport)
#define CHANNEL "AIDA64_SensorValues"
#define CAPACITY 16384
#define MAX_PIDS 128
typedef void *(__cdecl *QueryInterface)(uint32_t);
typedef int (__cdecl *NvInitialize)(void);
typedef int (__cdecl *NvUnload)(void);
typedef int (__cdecl *NvGetState)(void *);
typedef struct {
    DWORD pid;
    uint64_t sr,rr,fg;
    uint32_t preset,fg_preset,fg_count;
    int result;
    char name[MAX_PATH];
} Feedback;
static HMODULE nv_module;
static HANDLE stop_event,worker,mapping;
static char *sensor_view;
static NvGetState nv_get;
static NvUnload nv_unload;
static SRWLOCK info_lock=SRWLOCK_INIT;
static char info[2048]="DLSS provider is stopped.";
static DWORD last_pid;
static volatile LONG running;

static void set_info(const char *format,...) {
    va_list args;va_start(args,format);
    AcquireSRWLockExclusive(&info_lock);
    vsnprintf(info,sizeof info,format,args);
    ReleaseSRWLockExclusive(&info_lock);va_end(args);
}
static uint32_t read32(const unsigned char *p) { uint32_t n;memcpy(&n,p,4);return n; }
static uint64_t read64(const unsigned char *p) { uint64_t n;memcpy(&n,p,8);return n; }
static void put32(unsigned char *p,uint32_t n) { memcpy(p,&n,4); }
static int query_state(DWORD pid,Feedback *state) {
    unsigned char data[96];int version;
    memset(state,0,sizeof *state);state->pid=pid;
    for(version=2;version>=1;version--) {
        uint32_t size=(version==2)?96:64;
        memset(data,0,sizeof data);put32(data,size|((uint32_t)version<<16));put32(data+4,pid);
        state->result=nv_get(data);
        if(!state->result) {
            state->sr=read64(data+8);state->rr=read64(data+16);state->fg=read64(data+24);
            state->preset=read32(data+40);state->fg_count=read32(data+44);state->fg_preset=read32(data+48);
            return 1;
        }
        if(state->result!=-9)break; // NVAPI_INCOMPATIBLE_STRUCT_VERSION only.
    }
    return 0;
}
static void add_pid(DWORD *ids,unsigned *count,DWORD pid) {
    unsigned i;if(!pid || *count>=MAX_PIDS)return;
    for(i=0;i<*count;i++)if(ids[i]==pid)return;
    ids[(*count)++]=pid;
}
static unsigned candidates(DWORD *ids) {
    unsigned count=0;DWORD pid=0;
    HANDLE map;unsigned char *p;
    GetWindowThreadProcessId(GetForegroundWindow(),&pid);add_pid(ids,&count,pid);
    add_pid(ids,&count,last_pid);
    map=OpenFileMappingA(FILE_MAP_READ,FALSE,"RTSSSharedMemoryV2");
    if(!map)return count;
    p=MapViewOfFile(map,FILE_MAP_READ,0,0,0);
    if(p) {
        MEMORY_BASIC_INFORMATION memory;
        if(VirtualQuery(p,&memory,sizeof memory) && memory.RegionSize>=20 && read32(p)==0x52545353 && read32(p+4)>=0x20000) {
            uint32_t stride=read32(p+8),offset=read32(p+12),size=read32(p+16),i;
            if(stride>=4 && offset>=20 && size<=4096 && (uint64_t)offset+(uint64_t)stride*size<=memory.RegionSize) {
                for(i=0;i<size;i++)add_pid(ids,&count,read32(p+offset+(size_t)i*stride));
            }
        }
        UnmapViewOfFile(p);
    }
    CloseHandle(map);return count;
}
static void process_name(Feedback *state) {
    HANDLE process=OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,FALSE,state->pid);
    DWORD size=sizeof state->name;char *base;
    if(process) {
        if(!QueryFullProcessImageNameA(process,0,state->name,&size))state->name[0]=0;
        CloseHandle(process);
    }
    base=strrchr(state->name,'\\');
    if(base)memmove(state->name,base+1,strlen(base+1)+1);
    if(!state->name[0])snprintf(state->name,sizeof state->name,"PID %lu",state->pid);
    // Sensor XML is parsed by RTSS's small parser, not a general XML decoder.
    for(base=state->name;*base;base++)if(*base=='<' || *base=='>' || *base=='&')*base='_';
}
static int sensor(char *xml,size_t capacity,size_t *used,const char *id,const char *label,const char *value) {
    int n=snprintf(xml+*used,capacity-*used,"<sys><id>%s</id><label>%s</label><value>%s</value></sys>",id,label,value);
    if(n<0 || (size_t)n>=capacity-*used)return 0;
    *used+=(size_t)n;return 1;
}
static void feature(char *xml,size_t *used,const char *key,const char *name,uint64_t mask,uint32_t preset,int available) {
    char id[96],label[96],code[32],value[32];
    snprintf(id,sizeof id,"DLSS_%s_Override",key);snprintf(label,sizeof label,"DLSS %s override",name);
    sensor(xml,CAPACITY,used,id,label,available?dlss_state_name(dlss_state(mask)):"UNAVAILABLE");
    snprintf(id,sizeof id,"DLSS_%s_State",key);snprintf(label,sizeof label,"DLSS %s state code",name);
    snprintf(code,sizeof code,"%d",available?dlss_state(mask):-1);sensor(xml,CAPACITY,used,id,label,code);
    snprintf(id,sizeof id,"DLSS_%s_Preset",key);snprintf(label,sizeof label,"DLSS %s preset",name);
    if(available && dlss_state(mask)==DLSS_ON && (mask&32))dlss_preset(preset,value,sizeof value);
    else snprintf(value,sizeof value,"--");
    sensor(xml,CAPACITY,used,id,label,value);
}
static void publish(const Feedback *state,const char *notice) {
    char xml[CAPACITY]={0},combined[512],description[128],number[48];size_t used=0;
    int available=state && state->result==0;
    if(notice)snprintf(combined,sizeof combined,"DLSS override: %s",notice);
    else {
        dlss_describe(state->sr,state->preset,description,sizeof description);
        snprintf(combined,sizeof combined,"DLSS SR override: %s",description);
    }
    sensor(xml,sizeof xml,&used,"DLSSBridgeStatus","DLSS live status",combined);
    feature(xml,&used,"SR","SR",available?state->sr:0,available?state->preset:0,available);
    feature(xml,&used,"RR","RR",available?state->rr:0,available?state->preset:0,available);
    feature(xml,&used,"FG","FG",available?state->fg:0,available?state->fg_preset:0,available);
    sensor(xml,sizeof xml,&used,"DLSS_Process","DLSS target process",available?state->name:"--");
    snprintf(number,sizeof number,"%lu",available?state->pid:0);sensor(xml,sizeof xml,&used,"DLSS_PID","DLSS target PID",number);
    snprintf(number,sizeof number,"%d",available?state->result:-1);sensor(xml,sizeof xml,&used,"DLSS_API_Status","DLSS API status",number);
    // No active FG count is claimed if the FG override is inactive.
    snprintf(number,sizeof number,"%u",available && dlss_state(state->fg)==DLSS_ON?state->fg_count:0);
    sensor(xml,sizeof xml,&used,"DLSS_FG_Count","DLSS FG target count",number);
    if(sensor_view)memcpy(sensor_view,xml,used+1);
}
static int initialize_nvapi(void) {
    char path[MAX_PATH];QueryInterface query;NvInitialize init;int result;
    UINT n=GetSystemDirectoryA(path,sizeof path);
    if(!n || n+11>=sizeof path){set_info("Cannot locate the Windows system directory.");return 0;}
    strcat(path,"\\nvapi.dll"); // WOW64 resolves this to the installed 32-bit driver API.
    nv_module=LoadLibraryA(path);
    if(!nv_module){set_info("NVIDIA 32-bit driver API unavailable (Windows error %lu).",GetLastError());return 0;}
    query=(QueryInterface)(void*)GetProcAddress(nv_module,"nvapi_QueryInterface");
    if(!query){set_info("NVIDIA query interface unavailable.");return 0;}
    init=(NvInitialize)query(0x0150e828);nv_get=(NvGetState)query(0x3fd96fba);nv_unload=(NvUnload)query(0xd22bdd7e);
    if(!init || !nv_get){set_info("This NVIDIA driver does not expose the live NGX override API (R570+ required).");return 0;}
    result=init();
    if(result){set_info("NVIDIA API initialization failed: %d.",result);return 0;}
    return 1;
}
static DWORD WINAPI poll(LPVOID retained_module) {
    if(initialize_nvapi()) {
        do {
            DWORD ids[MAX_PIDS];unsigned count=candidates(ids),i;Feedback current;int found=0;
            for(i=0;i<count;i++) {
                if(WaitForSingleObject(stop_event,0)==WAIT_OBJECT_0)break;
                if(query_state(ids[i],&current) && (current.sr || current.rr || current.fg)) {found=1;break;}
            }
            if(found) {
                char sr[128],rr[128],fg[128];last_pid=current.pid;process_name(&current);
                publish(&current,NULL);
                dlss_describe(current.sr,current.preset,sr,sizeof sr);dlss_describe(current.rr,current.preset,rr,sizeof rr);dlss_describe(current.fg,current.fg_preset,fg,sizeof fg);
                set_info("RTSS DLSS 0.1.1 preview\r\n\r\nProcess: %s (PID %lu)\r\nSR override: %s\r\nRR override: %s\r\nFG override: %s\r\n\r\nLive NVIDIA API feedback: SR 0x%llX, RR 0x%llX, FG 0x%llX\r\nPolling every second. Data provider: AIDA-compatible.\r\n\r\nDisable this plugin to stop it. OFF refers to the override, not native DLSS.",current.name,current.pid,sr,rr,fg,(unsigned long long)current.sr,(unsigned long long)current.rr,(unsigned long long)current.fg);
            } else {
                last_pid=0;publish(NULL,"waiting for game");
                set_info("RTSS DLSS 0.1.1 preview\r\n\r\nRunning; waiting for a game with NVIDIA NGX feedback.\r\nEnable DLSS and your override in a game monitored by RTSS.\r\nNo feedback is not interpreted as DLSS OFF.");
            }
        } while(WaitForSingleObject(stop_event,1000)==WAIT_TIMEOUT);
        publish(NULL,"provider stopped");
        if(nv_unload)nv_unload();
    } else { publish(NULL,"driver API unavailable"); }
    if(nv_module){FreeLibrary(nv_module);nv_module=NULL;}nv_get=NULL;nv_unload=NULL;
    InterlockedExchange(&running,0);
    FreeLibraryAndExitThread((HMODULE)retained_module,0);
    return 0;
}
EXPORT BOOL __cdecl Start(void) {
    HMODULE retained=NULL;
    if(worker)return TRUE;
    SetLastError(0);mapping=CreateFileMappingA(INVALID_HANDLE_VALUE,NULL,PAGE_READWRITE,0,CAPACITY,CHANNEL);
    if(!mapping){set_info("Cannot create the sensor channel (Windows error %lu).",GetLastError());return FALSE;}
    if(GetLastError()==ERROR_ALREADY_EXISTS) {
        CloseHandle(mapping);mapping=NULL;
        set_info("The AIDA-compatible sensor channel is occupied.\r\n\r\nStop the earlier PowerShell DLSS bridge or AIDA64 shared-memory publisher, then re-enable RTSS DLSS. Existing sensor memory was not modified.");return FALSE;
    }
    sensor_view=MapViewOfFile(mapping,FILE_MAP_WRITE|FILE_MAP_READ,0,0,CAPACITY);
    if(!sensor_view){CloseHandle(mapping);mapping=NULL;set_info("Cannot map the sensor channel.");return FALSE;}
    stop_event=CreateEventA(NULL,TRUE,FALSE,NULL);
    if(!stop_event){UnmapViewOfFile(sensor_view);sensor_view=NULL;CloseHandle(mapping);mapping=NULL;return FALSE;}
    publish(NULL,"starting");set_info("RTSS DLSS is starting.");
    if(!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,(LPCSTR)(void*)&Start,&retained)) {
        CloseHandle(stop_event);stop_event=NULL;UnmapViewOfFile(sensor_view);sensor_view=NULL;CloseHandle(mapping);mapping=NULL;return FALSE;
    }
    InterlockedExchange(&running,1);
    worker=CreateThread(NULL,0,poll,retained,0,NULL);
    if(!worker) {
        InterlockedExchange(&running,0);FreeLibrary(retained);CloseHandle(stop_event);stop_event=NULL;
        UnmapViewOfFile(sensor_view);sensor_view=NULL;CloseHandle(mapping);mapping=NULL;set_info("Cannot start DLSS polling thread.");return FALSE;
    }
    return TRUE;
}
EXPORT void __cdecl Stop(void) {
    if(worker){SetEvent(stop_event);WaitForSingleObject(worker,INFINITE);CloseHandle(worker);worker=NULL;}
    if(stop_event){CloseHandle(stop_event);stop_event=NULL;}
    if(sensor_view){UnmapViewOfFile(sensor_view);sensor_view=NULL;}
    if(mapping){CloseHandle(mapping);mapping=NULL;}
    last_pid=0;set_info("RTSS DLSS is stopped. Re-enable the plugin to resume polling.");
}
EXPORT BOOL __cdecl Setup(HWND parent) {
    char text[2048];if(!parent)return TRUE;
    AcquireSRWLockShared(&info_lock);memcpy(text,info,sizeof text);ReleaseSRWLockShared(&info_lock);
    MessageBoxA(parent,text,"RTSS DLSS - live provider",MB_OK|MB_ICONINFORMATION);return TRUE;
}
BOOL WINAPI DllMain(HINSTANCE module,DWORD reason,LPVOID reserved) {
    (void)reserved;if(reason==DLL_PROCESS_ATTACH)DisableThreadLibraryCalls(module);return TRUE;
}
