#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <string.h>
typedef BOOL (__cdecl *StartFn)(void);
typedef void (__cdecl *StopFn)(void);
typedef BOOL (__cdecl *SetupFn)(HWND);
#define CHECK(x,msg) do {if(!(x)){fprintf(stderr,"FAIL: %s (Windows %lu)\n",msg,GetLastError());return 1;}}while(0)
int main(int argc,char **argv) {
    HMODULE plugin;StartFn start;StopFn stop;SetupFn setup;HANDLE map;char *view;int iteration;
    CHECK(argc>=2,"pass plugin DLL path");
    plugin=LoadLibraryA(argv[1]);CHECK(plugin,"load x86 plugin");
    start=(StartFn)(void*)GetProcAddress(plugin,"Start");stop=(StopFn)(void*)GetProcAddress(plugin,"Stop");setup=(SetupFn)(void*)GetProcAddress(plugin,"Setup");
    CHECK(start && stop && setup,"RTSS exports available");CHECK(setup(NULL),"configuration availability query");
    if(argc>=3 && !strcmp(argv[2],"--collision")) {
        map=CreateFileMappingA(INVALID_HANDLE_VALUE,NULL,PAGE_READWRITE,0,16384,"AIDA64_SensorValues");
        CHECK(map && GetLastError()!=ERROR_ALREADY_EXISTS,"test owns a fresh sensor mapping");
        view=MapViewOfFile(map,FILE_MAP_ALL_ACCESS,0,0,0);CHECK(view,"map sentinel memory");strcpy(view,"KEEP_EXISTING_PUBLISHER");
        CHECK(!start(),"occupied mapping refuses activation");CHECK(!strcmp(view,"KEEP_EXISTING_PUBLISHER"),"existing data stays untouched");
        stop();UnmapViewOfFile(view);CloseHandle(map);FreeLibrary(plugin);
        puts("PASS: occupied-channel refusal and data preservation");return 0;
    }
    for(iteration=0;iteration<3;iteration++) {
        CHECK(start(),"start native RTSS client");CHECK(start(),"duplicate start is idempotent");Sleep(2200);
        map=OpenFileMappingA(FILE_MAP_READ,FALSE,"AIDA64_SensorValues");CHECK(map,"sensor mapping exists while running");
        view=MapViewOfFile(map,FILE_MAP_READ,0,0,0);CHECK(view,"read published sensors");
        CHECK(strstr(view,"<id>DLSS_SR_Preset</id>"),"independent SR preset sensor");
        CHECK(strstr(view,"<id>DLSS_RR_Override</id>"),"independent RR status sensor");
        CHECK(strstr(view,"<id>DLSS_FG_Count</id>"),"independent FG count sensor");
        CHECK(strstr(view,"</sys>"),"complete sensor XML");
        if(iteration==0) {
            FILE *file=fopen("work/native-sensors.xml","wb");if(file){fwrite(view,1,strlen(view),file);fclose(file);}
            printf("Live sensor sample: %.180s\n",view);
            if(argc>=3 && !strcmp(argv[2],"--live")) {
                CHECK(strstr(view,"<label>DLSS API status</label><value>0</value>"),"successful live driver query");
                CHECK(strstr(view,"deadlock.exe"),"Deadlock selected as target");
            }
        }
        UnmapViewOfFile(view);CloseHandle(map);stop();stop();
        map=OpenFileMappingA(FILE_MAP_READ,FALSE,"AIDA64_SensorValues");
        CHECK(!map,"mapping released after stop");
    }
    FreeLibrary(plugin);puts("PASS: three native start/stop cycles, idempotence, source fields and channel cleanup");return 0;
}
