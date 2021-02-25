#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "misc.h"

void read_typechar_and_size(ubyte *bytes, char *type_char,unsigned int *size_int)
{
    memcpy(type_char,bytes,4);
    *(type_char + 4) = '\0';
    memcpy(size_int,bytes+4,4);
}

void read_typeint_and_size(ubyte *bytes, unsigned int *type_int,unsigned int *size_int)
{
    memcpy(type_int,bytes,4);
    memcpy(size_int,bytes+4,4);
}

unsigned int getUnixTime()
{
    return (unsigned int)time(NULL);
}
