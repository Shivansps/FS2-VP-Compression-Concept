#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "misc.h"
#include "vp.h"
#pragma warning(disable:4996)

void write_vp_header(FILE *vp,unsigned int index_offset, unsigned int num_files)
{
    char header[5]="VPVP";
    int version = 3; //2-Normal, 3-Compressed 

    if(vp!=NULL)
    {
        fseek(vp, 0, SEEK_SET);
        fwrite(header,1,4,vp);
        fwrite(&version,4,1,vp);
        fwrite(&index_offset,4,1,vp);
        fwrite(&num_files,4,1,vp);
/*
        printf("%d | %d \n",num_files,index_offset);
        getch();*/
    }
}

void write_vp_index_entry(FILE *vp, vp_index_entry index)
{
    if(vp!=NULL)
    {
        fwrite(&index.offset,4,1,vp);
        fwrite(&index.filesize,4,1,vp);
        fwrite(index.name,1,32,vp);
        fwrite(&index.timestamp,4,1,vp);
    }
}

void write_vp_file(FILE *vp, ubyte *file, char *name, unsigned int filesize, unsigned int timestamp, vp_index_entry *index, unsigned int *num_files, unsigned int *index_offset)
{

    if(vp!=NULL)
    {
        if(filesize==0&&timestamp==0)
        {
            /*FOLDER*/
            index[*num_files].filesize=0;
            index[*num_files].timestamp=0;
            index[*num_files].offset=0;
            strcpy(index[*num_files].name,name);
            (*num_files)++;
            write_vp_header(vp,*index_offset,*num_files);
            fseek(vp,*index_offset,SEEK_SET);
            for(int i=0;i<*num_files;i++)
                write_vp_index_entry(vp,index[i]);
        }
        else
        {
            /*FILE*/
            index[*num_files].filesize=filesize;
            index[*num_files].timestamp=timestamp;
            index[*num_files].offset=*index_offset;
            strcpy(index[*num_files].name,name);
            (*num_files)++;
            write_vp_header(vp,*index_offset,*num_files);
            fseek(vp,*index_offset,SEEK_SET);
            fwrite(file,sizeof(ubyte),filesize,vp);
            (*index_offset)+=filesize;
            for(int i=0;i<*num_files;i++)
                write_vp_index_entry(vp,index[i]);
        }
    }
}

ubyte* load_vp_file(FILE *vp, vp_index_entry vp_index_entry)
{
    if(vp!=NULL)
    {
        ubyte *file;
        file=(ubyte*)malloc(vp_index_entry.filesize);
        fseek(vp, 0, SEEK_SET);
        fseek(vp, vp_index_entry.offset, SEEK_SET);
        fread(file,vp_index_entry.filesize,1,vp);
        return file;
    }
    return NULL;
}

void load_vp_index(FILE *vp, vp_index_entry *vp_index_entry, unsigned int index_offset, unsigned int num_files)
{
    if(vp!=NULL)
    {
        char bytes[44];
        fseek(vp, 0, SEEK_SET);
        fseek(vp, index_offset, SEEK_SET);
        for(int x=0;x<num_files;x++)
        {
            fread(bytes,44,1,vp);
            memcpy(&vp_index_entry[x].offset,bytes,4);
            memcpy(&vp_index_entry[x].filesize,bytes+4,4);
            memset(&vp_index_entry[x].name,'\0',36);
            memcpy(&vp_index_entry[x].name,bytes+8,32);
            memcpy(&vp_index_entry[x].timestamp,bytes+40,4);
        }
    }
}

int read_vp_header(FILE *vp, char *header,unsigned int *version, unsigned int *index_offset, unsigned int *num_files)
{
    if(vp!=NULL)
    {
        fseek(vp, 0, SEEK_SET);
        fread(header,1,4,vp);
        fread(version,4,1,vp);
        fread(index_offset,4,1,vp);
        fread(num_files,4,1,vp);
        header[4]='\0';
        if(strcmp("VPVP",header)==0)
            return 1;
        else
            return 0;
    }
    return 0;
}
