#ifndef RTSS_DLSS_STATUS_H
#define RTSS_DLSS_STATUS_H
#include <stdint.h>
#include <stdio.h>
#include <string.h>
enum { DLSS_OFF=0, DLSS_PENDING=1, DLSS_ON=2, DLSS_ERROR=3 };
static int dlss_state(uint64_t mask) {
    if (!(mask&2)) return DLSS_OFF;
    if (mask&0x1f0000) return DLSS_ERROR;
    // An enabled driver override is not active while the game is not
    // evaluating DLSS (e.g. DLSS was switched off in the game's menu).
    return (mask&1024)?DLSS_ON:DLSS_OFF;
}
static const char *dlss_state_name(int state) {
    static const char *names[]={"OFF","PENDING","ON","ERROR"};
    return (state>=0 && state<=3)?names[state]:"UNAVAILABLE";
}
static void dlss_preset(uint32_t preset,char *out,size_t size) {
    if (preset>=1 && preset<=26) snprintf(out,size,"%c",'A'+preset-1);
    else if (preset==0xffffff) snprintf(out,size,"Latest");
    else snprintf(out,size,"%u",preset);
}
static void dlss_describe(uint64_t mask,uint32_t preset,char *out,size_t size) {
    int state=dlss_state(mask);
    if(state!=DLSS_ON) snprintf(out,size,"%s",dlss_state_name(state));
    else if(!(mask&32)) snprintf(out,size,"ON | Preset unavailable");
    else { char value[32];dlss_preset(preset,value,sizeof value);snprintf(out,size,"ON | Preset %s",value); }
}
#endif
