#include <iostream>
#include <lz4hc.h>
#include <lz4.h>
#include "vp.h"
#include <string>
#include <filesystem>
namespace fs = std::filesystem;
#pragma warning(disable:4996)

void compress_vp(FILE* vp_in, FILE* vp_out, int compression_level);
void decompress_vp(FILE* vp_in, FILE* vp_out);
void compress_folder(int compression_level);
void decompress_folder();
void compress_single_file_lz4(char* fn_in, char* fn_out, int compression_level);
void decompress_single_file_lz4(char* fn_in, char* fn_out);

/*.fc2 and .fs2 files crash FSO when read from memory, audio and movies should not be used this way and the rest is not recomended*/
#define LZ4_DO_NOT_COMPRESS ".wav .ogg .ani .eff .mve .mp4 .msb .srt .webm .fc2 .fs2"
#define LZ4_FILE_HEADER "CLZ4"
#define LZ4_MINIMUM_SIZE 20480

int main()
{
    int input;  
    char fn_in[200], fn_out[200];
    FILE* vp_in;
    FILE* vp_out;

    printf("Enter: \n 1 - To Compress all .vps files in the folder using LZ4.");  
    printf("\n 2 - To Compress all .vps files in the folder using LZ4-HC (very, very, veeery slow).");
    printf("\n 3 - To Decompress all .vps files in the folder.");
    printf("\n");
    printf("\n 4 - To Compress a single file using LZ4.");
    printf("\n 5 - To Compress a single file using LZ4-HC.");
    printf("\n 6 - To Decompress a single file.\n");
    printf("\n");
    printf("\n 7 - To Compress a single .VP file using LZ4.");
    printf("\n 8 - To Compress a single .VP file using LZ4-HC.");
    printf("\n 9 - To Decompress a single .VP file.\n");
    scanf("%d", &input);                   
    printf("\n");

    switch (input)
    {
        case 1 : compress_folder(5); break;
        case 2 : compress_folder(9); break;
        case 3 : decompress_folder(); break;
        case 4 : 
            printf("\n Enter filename (with ext): \n");
            scanf("%s", &fn_in);
            strcpy(fn_out, "compressed_");
            strcpy(fn_out+11, fn_in);
            compress_single_file_lz4(fn_in, fn_out, 5);
            break;
        case 5 :
            printf("\n Enter filename (with ext): \n");
            scanf("%s", &fn_in);
            strcpy(fn_out, "compressed_");
            strcpy(fn_out + 11, fn_in);
            compress_single_file_lz4(fn_in, fn_out, 9);
            break;
        case 6 :
            printf("\n Enter filename (with ext): \n");
            scanf("%s", &fn_in);
            strcpy(fn_out, "decompressed_");
            strcpy(fn_out + 13, fn_in);
            decompress_single_file_lz4(fn_in, fn_out);
            break;
        case 7:
            printf("\n Enter filename (with ext): \n");
            scanf("%s", &fn_in);
            printf("\nCompressing...\n");
            strcpy(fn_out, "compressed_");
            strcpy(fn_out + 11, fn_in);
            vp_in = fopen(fn_in, "rb");
            vp_out = fopen(fn_out, "wb");
            compress_vp(vp_in, vp_out, 5);
            break;
        case 8:
            printf("\n Enter filename (with ext): \n");
            scanf("%s", &fn_in);
            printf("\nCompressing...\n");
            strcpy(fn_out, "compressed_");
            strcpy(fn_out + 11, fn_in);
            vp_in = fopen(fn_in, "rb");
            vp_out = fopen(fn_out, "wb");
            compress_vp(vp_in, vp_out, 9);
            break;
        case 9:
            printf("\n Enter filename (with ext): \n");
            scanf("%s", &fn_in);
            printf("\nDecompressing...\n");
            strcpy(fn_out, "decompressed_");
            strcpy(fn_out + 13, fn_in);
            vp_in = fopen(fn_in, "rb");
            vp_out = fopen(fn_out, "wb");
            decompress_vp(vp_in, vp_out);
            break;
    }

    system("pause");
}

void compress_single_file_lz4(char* fn_in, char* fn_out, int compression_level)
{
    char* bytes_in, * bytes_out;
    int size, final_size;
    FILE* file_in = fopen(fn_in, "rb");
    FILE* file_out = fopen(fn_out, "wb");

    if (file_in != NULL)
    {
        fseek(file_in, 0, SEEK_END);
        size = ftell(file_in);
        std::cout << "Original Size: " << size << "\n";
        fseek(file_in, 0, SEEK_SET);
        bytes_in = (char*)malloc(size);
        fread(bytes_in, sizeof(char), size, file_in);
        fclose(file_in);
        unsigned int max_memory = LZ4_COMPRESSBOUND(size);
        std::cout << "Memory used: " << max_memory << "\n";
        bytes_out = (char*)malloc(max_memory);

        if (compression_level <= 5)
            final_size = LZ4_compress_default(bytes_in, bytes_out, size, max_memory);
        else
            final_size = LZ4_compress_HC(bytes_in, bytes_out, size, max_memory,compression_level);

        std::cout << "Compressed Size: " << final_size << "\n";
        fwrite(LZ4_FILE_HEADER, 1, 4, file_out); //Saving the char header
        fwrite(&size, sizeof(int), 1, file_out); //Saving the original file size
        fwrite(bytes_out, sizeof(char), final_size, file_out);
        fclose(file_out);
        free(bytes_in);
        free(bytes_out);
    }
    else
    {
        std::cout << "\n" << "Unable to open file : " << fn_in << "\n";
    }
}

void decompress_single_file_lz4(char* fn_in, char* fn_out)
{
    char* bytes_in, * bytes_out;
    char header[5];
    int size, final_size, result;
    FILE* file_in = fopen(fn_in, "rb");
    FILE* file_out = fopen(fn_out, "wb");
    if (file_in != NULL)
    {
        fseek(file_in, 0, SEEK_END);
        size = ftell(file_in);
        fseek(file_in, 0, SEEK_SET);
        bytes_in = (char*)malloc(size);
        header[4] = '\0';
        fread(header, 1, 4, file_in); //Read the header
        fread(&final_size, sizeof(int), 1, file_in); //Read the original file
        bytes_out = (char*)malloc(final_size);
        fread(bytes_in, 1, size, file_in);
        std::cout << "Header:" << header << "\n";
        std::cout << "Final Size:" << final_size;
        if (strcmp(header, LZ4_FILE_HEADER) == 0)
        {
            LZ4_decompress_fast(bytes_in, bytes_out, final_size);
            fwrite(bytes_out, 1, final_size, file_out);
            fclose(file_out);
        }
        else
        {
            std::cout << "\n" << "Unable to open file : " << fn_in << "\n" << "or file header does not match" << "\n";
        }
        fclose(file_in);
        free(bytes_in);
        free(bytes_out);
    }
}

void compress_vp(FILE* vp_in, FILE* vp_out, int compression_level)
{
    char header[5];
    unsigned int version, index_offset, numfiles;

    if (!read_vp_header(vp_in, header, &version, &index_offset, &numfiles) == 1)
    {
        printf(" Not a VP file.\n");
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
            char* file_extension = strchr(index[x].name, '.');
            ubyte* file;
            file = load_vp_file(vp_in, index[x]);
            if (!strstr(LZ4_DO_NOT_COMPRESS, file_extension)&&index[x].filesize>=LZ4_MINIMUM_SIZE)
            {
                ubyte* compressed_bytes;
                unsigned int max_memory = LZ4_COMPRESSBOUND(index[x].filesize);
                compressed_bytes = (ubyte*)malloc(max_memory);

                memcpy(compressed_bytes, LZ4_FILE_HEADER, 4);
                memcpy(compressed_bytes+4, &index[x].filesize, sizeof(int));

                int newsize;
                if (compression_level <= 5)
                    newsize = LZ4_compress_default(file, compressed_bytes + 8, index[x].filesize, max_memory); // Normal LZ4, fast compression, fast decompression, somewhat poor compression ratio in some cases.
                else
                    newsize = LZ4_compress_HC(file, compressed_bytes + 8, index[x].filesize, max_memory, compression_level); // High-Compression with better ratio, super very slow compression speed, it needs a MT implementation.

                write_vp_file(vp_out, compressed_bytes, index[x].name, newsize + 8, getUnixTime(), index_out, &wvp_num_files, &wvp_index_offset);

                free(file);
                free(compressed_bytes);
            }
            else
            {
                write_vp_file(vp_out, file, index[x].name, index[x].filesize, getUnixTime(), index_out, &wvp_num_files, &wvp_index_offset);
                free(file);
            }
            count++;
        }
        else
        {
            write_vp_file(vp_out, NULL, index[x].name, index[x].filesize, index[x].timestamp, index_out, &wvp_num_files, &wvp_index_offset);
            count++;
        }
    }
}

void decompress_vp(FILE* vp_in, FILE* vp_out)
{
    char header[5];
    unsigned int version, index_offset, numfiles, count = 1, wvp_num_files = 0, wvp_index_offset = 16;

    if (!read_vp_header(vp_in, header, &version, &index_offset, &numfiles) == 1)
    {
        printf(" Not a VP file.\n");
        return;
    }

    vp_index_entry* index;
    index = (vp_index_entry*)malloc(sizeof(vp_index_entry) * numfiles);
    load_vp_index(vp_in, index, index_offset, numfiles);

    vp_index_entry* index_out;
    index_out = (vp_index_entry*)malloc(sizeof(vp_index_entry) * numfiles);

    for (int x = 0; x < numfiles; x++)
    {
        if (index[x].offset != 0 && index[x].filesize != 0 && index[x].timestamp != 0)
        {
            ubyte* file;
            file = load_vp_file(vp_in, index[x]);
            int original_size;
            ubyte* uncompressed_bytes;
            char file_header[5];
            file_header[4] = '\0';

            memcpy(file_header, file, 4);
            memcpy(&original_size, file+4, sizeof(int));

            if (strcmp(file_header, LZ4_FILE_HEADER) == 0)
            {
                uncompressed_bytes = (ubyte*)malloc(original_size);
                LZ4_decompress_fast(file+8, uncompressed_bytes, original_size);
                write_vp_file(vp_out, uncompressed_bytes, index[x].name, original_size, getUnixTime(), index_out, &wvp_num_files, &wvp_index_offset);
                free(file);
                free(uncompressed_bytes);
            }
            else
            {
                write_vp_file(vp_out, file, index[x].name, index[x].filesize, getUnixTime(), index_out, &wvp_num_files, &wvp_index_offset);
                free(file);
            }
            count++;
        }
        else
        {
            write_vp_file(vp_out, NULL, index[x].name, index[x].filesize, index[x].timestamp, index_out, &wvp_num_files, &wvp_index_offset);
            count++;
        }
    }
}

void compress_folder(int compression_level)
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
                compress_vp(file_in, file_out, compression_level);
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