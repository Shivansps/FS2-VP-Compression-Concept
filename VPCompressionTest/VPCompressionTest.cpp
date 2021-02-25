#include <iostream>
#include <lz4.h>
#include "vp.h"
#include <string>
#include <filesystem>
namespace fs = std::filesystem;
#pragma warning(disable:4996)

void compress_vp(FILE* vp_in, FILE* vp_out);
void decompress_vp(FILE* vp_in, FILE* vp_out);
void compress_folder();
void decompress_folder();

int main()
{

    compress_folder();
    std::cout << "\nFinished, press enter to close";
    getchar();
    
    /*
    //Compress Single File
    char* bytes_in, * bytes_out;
    int size;
    FILE* file_in = fopen("D:\\mv_effects.vp", "rb");
    FILE* file_out = fopen("D:\\mv_effects_compressed.vp", "wb");
    if (file_in != NULL)
    {
        fseek(file_in, 0, SEEK_END);
        size = ftell(file_in);
        std::cout << size ;
        fseek(file_in, 0, SEEK_SET);
        bytes_in = (char*)malloc(size);
        fread(bytes_in, sizeof(char), size, file_in);
        fclose(file_in);
        bytes_out = (char*)malloc(size*2);
        int final_size = LZ4_compress_default(bytes_in, bytes_out, size, size * 2);
        std::cout << final_size;
        fwrite(&size, sizeof(int), 1, file_out); //Saving the original file size in the first 4 bytes, it is needed to decompress
        fwrite(bytes_out, sizeof(char), final_size, file_out);
        fclose(file_out);
        free(bytes_in);
        free(bytes_out);
    }
    */
    /*
    //Decompress Single File
    char* bytes_in, * bytes_out;
    int size;
    FILE* file_in = fopen("D:\\Thunder_01.wav", "rb");
    FILE* file_out = fopen("D:\\Thunder_01_decompressed.wav", "wb");
    if (file_in != NULL)
    {
        fseek(file_in, 0, SEEK_END);
        size = ftell(file_in);
        fseek(file_in, 0, SEEK_SET);
        bytes_in = (char*)malloc(size);
        int final_size;
        fread(&final_size, sizeof(int), 1, file_in); //Reading the original file size in the first 4 bytes, it is needed to decompress
        fread(bytes_in, sizeof(char), size, file_in);
        fclose(file_in);
        std::cout << final_size;
        bytes_out = (char*)malloc(final_size);
        LZ4_decompress_fast(bytes_in, bytes_out, final_size);
        fwrite(bytes_out, sizeof(char), final_size, file_out);
        fclose(file_out);
        free(bytes_in);
        free(bytes_out);
    }
    */
}

void compress_vp(FILE* vp_in, FILE* vp_out)
{
    char header[5];
    unsigned int version, index_offset, numfiles;

    if (!read_vp_header(vp_in, header, &version, &index_offset, &numfiles) == 1)
    {
        printf(" Not a VP file.\n");
        return;
    }

    if (version > 2)
    {
        printf(" This VP is already compressed, skipping...\n");
        return;
    }

    vp_index_entry* index;
    index = (vp_index_entry*)malloc(sizeof(vp_index_entry) * numfiles);
    load_vp_index(vp_in, index, index_offset, numfiles);

    int count = 1;
    unsigned int wvp_num_files = 0, wvp_index_offset = 16;
    vp_index_entry* index_out;
    index_out = (vp_index_entry*)malloc(sizeof(vp_index_entry) * numfiles);

    for (int x = 0; x < numfiles; x++)
    {
        if (index[x].offset != 0 && index[x].filesize != 0 && index[x].timestamp != 0)
        {
            //printf("\nFILE %d/%d | NAME: %s\n", count, numfiles, index[x].name);
            ubyte* file;
            file = load_vp_file(vp_in, index[x]);
            ubyte* compressed_bytes;

            compressed_bytes = (ubyte*)malloc(index[x].filesize * 2);

            memcpy(compressed_bytes, &index[x].filesize, sizeof(int));

            int newsize = LZ4_compress_default(file, compressed_bytes + sizeof(int), index[x].filesize, index[x].filesize * 2);

            write_vp_file(vp_out, compressed_bytes, index[x].name, newsize + sizeof(int), getUnixTime(), index_out, &wvp_num_files, &wvp_index_offset);

            free(file);

            count++;
        }
        else
        {
            //printf("\nFILE %d/%d | NAME: %s\n", count, numfiles, index[x].name);
            write_vp_file(vp_out, NULL, index[x].name, index[x].filesize, index[x].timestamp, index_out, &wvp_num_files, &wvp_index_offset);
            count++;
        }
    }
}

void decompress_vp(FILE* vp_in, FILE* vp_out)
{
    char header[5];
    unsigned int version, index_offset, numfiles;

    if (!read_vp_header(vp_in, header, &version, &index_offset, &numfiles) == 1)
    {
        printf(" Not a VP file.\n");
        return;
    }

    if (version < 3)
    {
        printf(" This VP is not compressed, skipping...\n");
        return;
    }

    vp_index_entry* index;
    index = (vp_index_entry*)malloc(sizeof(vp_index_entry) * numfiles);
    load_vp_index(vp_in, index, index_offset, numfiles);

    int count = 1;
    unsigned int wvp_num_files = 0, wvp_index_offset = 16;
    vp_index_entry* index_out;
    index_out = (vp_index_entry*)malloc(sizeof(vp_index_entry) * numfiles);

    for (int x = 0; x < numfiles; x++)
    {
        if (index[x].offset != 0 && index[x].filesize != 0 && index[x].timestamp != 0)
        {
            //printf("\nFILE %d/%d | NAME: %s\n", count, numfiles, index[x].name);
            ubyte* file;
            file = load_vp_file(vp_in, index[x]);
            int original_size;
            ubyte* uncompressed_bytes;

            memcpy(&original_size, file, sizeof(int));

            uncompressed_bytes = (ubyte*)malloc(original_size);

            LZ4_decompress_fast(file + sizeof(int), uncompressed_bytes, original_size);

            write_vp_file(vp_out, uncompressed_bytes, index[x].name, original_size, getUnixTime(), index_out, &wvp_num_files, &wvp_index_offset);

            free(file);

            count++;
        }
        else
        {
            //printf("\nFILE %d/%d | NAME: %s\n", count, numfiles, index[x].name);
            write_vp_file(vp_out, NULL, index[x].name, index[x].filesize, index[x].timestamp, index_out, &wvp_num_files, &wvp_index_offset);
            count++;
        }
    }
}

void compress_folder()
{
    for (const auto& entry : fs::directory_iterator(fs::current_path()))
    {
        if (fs::is_regular_file(entry) && entry.path().extension() == ".vp")
        {
            std::string path = entry.path().string();
            std::string pathout = path.substr(0, path.length() - 3).append("_compressed.vp");

            FILE* file_in = fopen(path.c_str(), "rb");
            FILE* file_out = fopen(pathout.c_str(), "wb");
            if (file_in != NULL)
            {
                std::cout << "\nCOMPRESSING FILE : " <<entry.path().filename();
                compress_vp(file_in, file_out);
                fclose(file_in);

                int size = ftell(file_out);
                fclose(file_out);
                if(size==0)
                    remove(pathout.c_str());
            }
        }
    }
}

void decompress_folder()
{
    for (const auto& entry : fs::directory_iterator(fs::current_path()))
    {
        if (fs::is_regular_file(entry) && entry.path().extension() == ".vp")
        {
            std::string path = entry.path().string();
            std::string pathout = path.substr(0, path.length() - 3).append("_decompressed.vp");

            FILE* file_in = fopen(path.c_str(), "rb");
            FILE* file_out = fopen(pathout.c_str(), "wb");
            if (file_in != NULL)
            {
                std::cout << "\nDE-COMPRESSING FILE : " << entry.path().filename();
                decompress_vp(file_in, file_out);
                fclose(file_in);
                int size = ftell(file_out);
                fclose(file_out);
                if (size == 0)
                    remove(pathout.c_str());
            }
        }
    }
}