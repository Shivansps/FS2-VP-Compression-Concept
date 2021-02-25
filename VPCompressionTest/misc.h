#ifndef GENERAL_H_INCLUDED
#define GENERAL_H_INCLUDED

#define VERSION "0.3"

//FSO Compatibility
typedef char ubyte;

/*NOTE: vec3d size is 12, 3 floats inside*/

/*Char must be size 5*/
void read_typechar_and_size(ubyte *,char*,unsigned int*);
void read_typeint_and_size(ubyte *,unsigned int*,unsigned int*);

unsigned int getUnixTime();

#endif // GENERAL_H_INCLUDED
