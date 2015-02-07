#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cfgParser.h"

int main(int argc, char **argv) {
    int result = 0;

    char out[1024];
    
    result = cfgParse("test.ini");
    if (result != 0) return 1;
    
    printf("\n==================GET RESULT:GLOBAL======================\n");
    memset(out, 0x00, sizeof(out));
    cfgGet(NULL, "aa", out);
    printf("Global section, aa=[%s]\n", out);
    
    memset(out, 0x00, sizeof(out));
    cfgGet(NULL, "a", out);
    printf("Global section, a=[%s]\n", out);
    
    memset(out, 0x00, sizeof(out));
    cfgGet(NULL, "b", out);
    printf("Global section, b=[%s]\n", out);
    
    memset(out, 0x00, sizeof(out));
    cfgGet(NULL, "c", out);
    printf("Global section, c=[%s]\n", out);
    
    printf("\n==================GET RESULT:[ab;cdefg]======================\n");
    memset(out, 0x00, sizeof(out));
    cfgGet("ab;cdefg", "c", out);
    printf("Named section [ab;cdefg], c=[%s]\n", out);
    
    memset(out, 0x00, sizeof(out));
    cfgGet("ab;cdefg", "d", out);
    printf("Named section [ab;cdefg], d=[%s]\n", out);
    
    memset(out, 0x00, sizeof(out));
    cfgGet("ab;cdefg", "e", out);
    printf("Named section [ab;cdefg], e=[%s]\n", out);
    
    printf("\n==================GET RESULT:[xxxx]======================\n");
    memset(out, 0x00, sizeof(out));
    cfgGet("xxxxa", "e", out);
    printf("Named section [xxxx], e=[%s]\n", out);
    
    memset(out, 0x00, sizeof(out));
    cfgGet("xxxx", "m", out);
    printf("Named section [xxxx], m=[%s]\n", out);
    
    memset(out, 0x00, sizeof(out));
    cfgGet("xxxx", "n", out);
    printf("Named section [xxxx], n=[%s]\n", out);
    
    //cfgPrint();
    cfgFree();
    return result;
}

