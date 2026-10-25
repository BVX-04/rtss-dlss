#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "status.h"
int main(void) {
    char out[128];
    assert(dlss_state(0x63f)==DLSS_ON);
    assert(dlss_state(3)==DLSS_OFF);
    assert(dlss_state(0x3f)==DLSS_OFF);
    assert(dlss_state(0x23f)==DLSS_OFF);
    assert(dlss_state(0x1063f)==DLSS_ERROR);
    assert(dlss_state(0x90001)==DLSS_OFF);
    assert(dlss_state(0)==DLSS_OFF);
    dlss_preset(13,out,sizeof out); assert(!strcmp(out,"M"));
    dlss_preset(0xffffff,out,sizeof out); assert(!strcmp(out,"Latest"));
    dlss_preset(99,out,sizeof out); assert(!strcmp(out,"99"));
    dlss_describe(0x63f,13,out,sizeof out); assert(!strcmp(out,"ON | Preset M"));
    dlss_describe(0x41f,0,out,sizeof out); assert(!strcmp(out,"ON | Preset unavailable"));
    dlss_describe(3,13,out,sizeof out); assert(!strcmp(out,"OFF"));
    puts("PASS: 13 native status checks");
    return 0;
}
