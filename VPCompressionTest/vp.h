#ifndef VP_H_INCLUDED
#define VP_H_INCLUDED

#include "misc.h"

typedef struct vp_index_entry {
    unsigned int offset;
    unsigned int filesize;
    char name[32];
    unsigned int timestamp;
}vp_index_entry;

typedef struct file_vp {
    ubyte *data;
    unsigned int type; //0 folder // 1 file
}file_vp;

/*Header char must be of size 5, Returns 0 if file is not a VP*/
int read_vp_header(FILE *vp, char *header,unsigned int *version, unsigned int *index_offset, unsigned int *num_files);
void load_vp_index(FILE *vp, vp_index_entry *vp_index_entry, unsigned int index_offset,  unsigned int num_files);
ubyte* load_vp_file(FILE *vp, vp_index_entry vp_index_entry);

void write_vp_header(FILE *vp,unsigned int index_offset, unsigned int num_files);
void write_vp_index_entry(FILE *vp, vp_index_entry vp_index_entry);
void write_vp_file(FILE *vp, ubyte *file, char *name, unsigned int filesize, unsigned int timestamp, vp_index_entry *index, unsigned int *num_files, unsigned int *index_offset);

#endif // VP_H_INCLUDED
