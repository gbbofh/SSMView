#include "util.h"

#include <stdlib.h>

#include <time.h>
#include <string.h>

char* generate_guid()
{
    size_t len;
    char* guid;
    char* base = "xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx";
    char* hex = "0123456789abcdef";

    len = strlen(base) + 1;
    guid = calloc(len, sizeof(char));

    srand(time(NULL));

    for(int i = 0; i < len; i++) {

        int r;

        r = rand() % 16;

        switch(base[i]) {

            case 'x': { guid[i] = hex[r]; } break;
            case 'y': { guid[i] = hex[(r & 0x03) | 0x08]; } break;
            case '-': { guid[i] = '-'; } break;
            case '4': { guid[i] = '4'; } break;
        }
    }

    return guid;
}


