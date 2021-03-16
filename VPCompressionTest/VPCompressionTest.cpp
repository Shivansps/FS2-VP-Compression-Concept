#include <iostream>
#include <lz4hc.h>
#include <lz4.h>
#include "vp.h"
#include <string>
#include <filesystem>
namespace fs = std::filesystem;
#pragma warning(disable:4996)

/* ShivanSpS - I modified the LZ4 random access example with dictionary as a base for this implementation. 
-Mission files and tables should stay in a readeable format, you can compress then, but i think it is better this way.
-The minimum size is a optimization, there is no need to compress very small files.
-The file header is a version, this is used to tell FSO how to decompress that file, always use 4 chars to mantain alignment, it is stored at the start of the file. "LZ41" for this implementation.
-The block size is used to tell how much information is compressed into a block, each block adds overhead, so a larger the block bytes result in smaller file size.
-COMPRESSED FILE DATA STUCTURE: HEADER|BLOCKS|OFFSETS|NUM_OFFSETS|ORIGINAL_FILESIZE|BLOCK_SIZE
*/
#define LZ41_DO_NOT_COMPRESS "" //".png .ogg .fs2 .fc2 .tbl .tbm"
#define LZ41_MINIMUM_SIZE 20480
#define LZ41_FILE_HEADER "LZ41"
#define LZ41_BLOCK_BYTES 65536

/*RETURN ERROR CODES*/
#define LZ41_DECOMPRESSION_ERROR -1
#define LZ41_MAX_BLOCKS_OVERFLOW -2
#define LZ41_HEADER_MISMATCH -3
#define LZ41_OFFSETS_MISMATCH -4
#define LZ41_COMPRESSION_ERROR -5

void compress_vp(FILE* vp_in, FILE* vp_out, int compression_level);
void decompress_vp(FILE* vp_in, FILE* vp_out);
void compress_folder(int compression_level);
void decompress_folder();
void compress_single_file_lz4(char* fn_in, char* fn_out, int compression_level);
void decompress_single_file_lz4(char* fn_in, char* fn_out);

/*LZ4 implementation*/
int lz41_stream_compress(FILE* file_in, FILE* file_out);
int lz41_stream_decompress(FILE* file_in, FILE* file_out);
int lz41_compress_memory(char* bytes_in, char* bytes_out, int file_size);
int lz41_decompress_memory(char* bytes_in, char* bytes_out, int compressed_size, int original_size);
int lz41_stream_read_random_access(FILE* file_in, char* bytes_out, int offset, int length);

/*LZ4-HC implementation, better ratio but slower, Compression level: 5 to 12 (MAX) */
int lz41_stream_compress_HC(FILE* file_in, FILE* file_out, int compression_level);
int lz41_compress_memory_HC(char* bytes_in, char* bytes_out, int file_size, int compression_level);

int lz41_compress_memory(char* bytes_in, char* bytes_out, int file_size)
{
    LZ4_stream_t lz4Stream_body;
    LZ4_stream_t* lz4Stream = &lz4Stream_body;

    char inpBuf[LZ41_BLOCK_BYTES];
    int max_blocks = file_size + sizeof(int) / LZ41_BLOCK_BYTES;
    int* offsets = (int*)malloc(max_blocks);
    int* offsetsEnd = offsets;
    int in_offset = 0;
    int written_bytes = 0;

    LZ4_initStream(lz4Stream, sizeof(*lz4Stream));

    /* Write header */
    memcpy(bytes_out, LZ41_FILE_HEADER, sizeof(LZ41_FILE_HEADER));
    bytes_out += sizeof(LZ41_FILE_HEADER);
    written_bytes += sizeof(LZ41_FILE_HEADER);
    *offsetsEnd++ = sizeof(LZ41_FILE_HEADER);

    /* Write compressed data blocks.  Each block contains BLOCK_BYTES of plain
       data except possibly the last. */
    while(in_offset < file_size)
    {
        int amountToRead = LZ41_BLOCK_BYTES;

        if (file_size < in_offset+amountToRead)
            amountToRead -= (in_offset + amountToRead) - file_size;

        memcpy(inpBuf, bytes_in, amountToRead);
        bytes_in += amountToRead;
        in_offset += amountToRead;

        LZ4_resetStream(lz4Stream);

        char cmpBuf[LZ4_COMPRESSBOUND(LZ41_BLOCK_BYTES)];
        int cmpBytes = LZ4_compress_fast_continue(lz4Stream, inpBuf, cmpBuf, amountToRead, sizeof(cmpBuf), 1);
        if (cmpBytes <= 0)
            return LZ41_COMPRESSION_ERROR;

        memcpy(bytes_out, cmpBuf, cmpBytes);
        bytes_out += cmpBytes;
        written_bytes += cmpBytes;
        
        /* Keep track of the offsets */
        *offsetsEnd = *(offsetsEnd - 1) + cmpBytes;
        ++offsetsEnd;

        if (offsetsEnd - offsets > max_blocks)
            return LZ41_MAX_BLOCKS_OVERFLOW;
    }

    /* Write the tailing jump table */
    int* ptr = offsets;
    while (ptr != offsetsEnd)
    {
        memcpy(bytes_out, ptr++, sizeof(int));
        bytes_out += sizeof(int);
        written_bytes += sizeof(int);
    }

    /* Write number of offsets */
    int i = offsetsEnd - offsets;
    memcpy(bytes_out, &i, sizeof(int));
    bytes_out += sizeof(int);
    written_bytes += sizeof(int);

    /* Write Filesize */
    memcpy(bytes_out, &file_size, sizeof(int));
    bytes_out += sizeof(int);
    written_bytes += sizeof(int);

    /* Write Block Size */
    int bb = LZ41_BLOCK_BYTES;
    memcpy(bytes_out, &bb, sizeof(int));
    bytes_out += sizeof(int);
    written_bytes += sizeof(int);

    free(offsets);
    return written_bytes;
}

int lz41_compress_memory_HC(char* bytes_in, char* bytes_out, int file_size, int compression_level)
{
    LZ4_streamHC_t lz4Stream_body;
    LZ4_streamHC_t* lz4Stream = &lz4Stream_body;

    char inpBuf[LZ41_BLOCK_BYTES];
    int max_blocks = file_size + sizeof(int) / LZ41_BLOCK_BYTES;
    int* offsets = (int*)malloc(max_blocks);
    int* offsetsEnd = offsets;
    int in_offset = 0;
    int written_bytes = 0;

    LZ4_initStream(lz4Stream, sizeof(*lz4Stream));

    /* Write header */
    memcpy(bytes_out, LZ41_FILE_HEADER, sizeof(LZ41_FILE_HEADER));
    bytes_out += sizeof(LZ41_FILE_HEADER);
    written_bytes += sizeof(LZ41_FILE_HEADER);
    *offsetsEnd++ = sizeof(LZ41_FILE_HEADER);

    /* Write compressed data blocks.  Each block contains BLOCK_BYTES of plain
       data except possibly the last. */
    while (in_offset < file_size)
    {
        int amountToRead = LZ41_BLOCK_BYTES;

        if (file_size < in_offset + amountToRead)
            amountToRead -= (in_offset + amountToRead) - file_size;

        memcpy(inpBuf, bytes_in, amountToRead);
        bytes_in += amountToRead;
        in_offset += amountToRead;

        LZ4_resetStreamHC(lz4Stream,compression_level);

        char cmpBuf[LZ4_COMPRESSBOUND(LZ41_BLOCK_BYTES)];
        int cmpBytes = LZ4_compress_HC_continue(lz4Stream, inpBuf, cmpBuf, amountToRead, sizeof(cmpBuf));
        if (cmpBytes <= 0)
            return LZ41_COMPRESSION_ERROR;

        memcpy(bytes_out, cmpBuf, (size_t)cmpBytes);
        bytes_out += (size_t)cmpBytes;
        written_bytes += (size_t)cmpBytes;

        /* Keep track of the offsets */
        *offsetsEnd = *(offsetsEnd - 1) + cmpBytes;
        ++offsetsEnd;

        if (offsetsEnd - offsets > max_blocks)
            return LZ41_MAX_BLOCKS_OVERFLOW;
    }

    /* Write the tailing jump table */
    int* ptr = offsets;
    while (ptr != offsetsEnd)
    {
        memcpy(bytes_out, ptr++, sizeof(int));
        bytes_out += sizeof(int);
        written_bytes += sizeof(int);
    }

    /* Write number of offsets */
    int i = offsetsEnd - offsets;
    memcpy(bytes_out, &i, sizeof(int));
    bytes_out += sizeof(int);
    written_bytes += sizeof(int);

    /* Write Filesize */
    memcpy(bytes_out, &file_size, sizeof(int));
    bytes_out += sizeof(int);
    written_bytes += sizeof(int);

    /* Write Block Size */
    int bb = LZ41_BLOCK_BYTES;
    memcpy(bytes_out, &bb, sizeof(int));
    bytes_out += sizeof(int);
    written_bytes += sizeof(int);

    free(offsets);
    return written_bytes;
}

int lz41_decompress_memory(char* bytes_in, char* bytes_out,int compressed_size, int original_size)
{
    LZ4_streamDecode_t lz4StreamDecode_body;
    LZ4_streamDecode_t* lz4StreamDecode = &lz4StreamDecode_body;
    int offset = 0;
    int length = original_size;
    int written_bytes = 0;
    int block_size;
    memcpy(&block_size, bytes_in + compressed_size - 4, sizeof(int));

    char *decBuf=(char*)malloc(block_size);
    int max_blocks = length + 8 / block_size;
    int* offsets = (int*)malloc(max_blocks);

    /*Read Header*/
    char header[sizeof(LZ41_FILE_HEADER)];
    memcpy(header, bytes_in, sizeof(LZ41_FILE_HEADER));
    if (memcmp(LZ41_FILE_HEADER, header, sizeof(LZ41_FILE_HEADER)))
        return LZ41_HEADER_MISMATCH;

    /* The blocks [currentBlock, endBlock) contain the data we want */
    int currentBlock = offset / block_size;
    int endBlock = ((offset + length - 1) / block_size) + 1;

    /* Special cases */
    if (length == 0)
        return 0;

    /* Read the offsets tail */
    int numOffsets;
    int block;
    int* offsetsPtr = offsets;
    memcpy(&numOffsets, bytes_in + compressed_size - 12, sizeof(int));
    if (numOffsets <= endBlock)
        return LZ41_OFFSETS_MISMATCH;
    
    char* firstOffset = bytes_in + compressed_size + (-4 * (numOffsets + 1)) - (sizeof(int)*2);
    for (block = 0; block <= endBlock; ++block)
    {
        memcpy(offsetsPtr++, firstOffset, sizeof(int));
        firstOffset += sizeof(int);
    }

    /* Seek to the first block to read */
    bytes_in +=offsets[currentBlock];
    offset = offset % block_size;

    /* Start decoding */
    for (; currentBlock < endBlock; ++currentBlock)
    {
        char* cmpBuf = (char*)malloc(LZ4_compressBound(block_size));
        /* The difference in offsets is the size of the block */
        int  cmpBytes = offsets[currentBlock + 1] - offsets[currentBlock];
        memcpy(cmpBuf, bytes_in, cmpBytes);
        bytes_in += cmpBytes;

        const int decBytes = LZ4_decompress_safe_continue(lz4StreamDecode, cmpBuf, decBuf, cmpBytes, LZ41_BLOCK_BYTES);
        if (decBytes <= 0)
            return LZ41_DECOMPRESSION_ERROR;

        /* Write out the part of the data we care about */
        int blockLength = ((length) < ((decBytes - offset)) ? (length) : ((decBytes - offset)));
        memcpy(bytes_out + written_bytes, decBuf + offset, (size_t)blockLength);
        written_bytes += (size_t)blockLength;
        offset = 0;
        length -= blockLength;
        free(cmpBuf);
    }
    free(decBuf);
    free(offsets);
    return written_bytes;
}

int lz41_stream_read_random_access(FILE* file_in, char* bytes_out, int offset, int length)
{
    LZ4_streamDecode_t lz4StreamDecode_body;
    LZ4_streamDecode_t* lz4StreamDecode = &lz4StreamDecode_body;
    int written_bytes = 0;
    int numOffsets;
    int block_size;

    /* Num Offsets */
    fseek(file_in, sizeof(int) * -3, SEEK_END);
    fread(&numOffsets, sizeof(int), 1, file_in);

    /* Block Size */
    fseek(file_in, sizeof(int) * -1, SEEK_END);
    fread(&block_size, sizeof(int), 1, file_in);

    /* The blocks [currentBlock, endBlock) contain the data we want */
    int currentBlock = offset / block_size;
    int endBlock = ((offset + length - 1) / block_size) + 1;

    char* decBuf = (char*)malloc(block_size);
    fseek(file_in, 0, SEEK_END);
    int max_blocks = ftell(file_in) + 8 / block_size;
    int* offsets = (int*)malloc(max_blocks);
    fseek(file_in, 0, SEEK_SET);

    /* Special cases */
    if (length == 0)
        return 0;

    /*Read Header*/
    char header[sizeof(LZ41_FILE_HEADER)];
    fread(header, 1, sizeof(LZ41_FILE_HEADER), file_in);
    if (memcmp(LZ41_FILE_HEADER, header, sizeof(LZ41_FILE_HEADER)))
        return LZ41_HEADER_MISMATCH;

    /* Read the offsets tail */
    int block;
    int* offsetsPtr = offsets;

    if (numOffsets <= endBlock)
        return LZ41_OFFSETS_MISMATCH;

    fseek(file_in, (-4 * (numOffsets + 1)) - (sizeof(int) * 2), SEEK_END);
    for (block = 0; block <= endBlock; ++block)
        fread(offsetsPtr++, sizeof(int), 1, file_in);

    /* Seek to the first block to read */
    fseek(file_in, offsets[currentBlock], SEEK_SET);
    offset = offset % block_size;

    /* Start decoding */
    for (; currentBlock < endBlock; ++currentBlock)
    {
        char* cmpBuf = (char*)malloc(LZ4_compressBound(block_size));
        /* The difference in offsets is the size of the block */
        int  cmpBytes = offsets[currentBlock + 1] - offsets[currentBlock];
        fread(cmpBuf, (size_t)cmpBytes, 1, file_in);

        const int decBytes = LZ4_decompress_safe_continue(lz4StreamDecode, cmpBuf, decBuf, cmpBytes, block_size);
        if (decBytes <= 0)
            return LZ41_DECOMPRESSION_ERROR;

        /* Write out the part of the data we care about */
        int blockLength = ((length) < ((decBytes - offset)) ? (length) : ((decBytes - offset)));
        memcpy(bytes_out + written_bytes, decBuf + offset, (size_t)blockLength);
        written_bytes += (size_t)blockLength;
        offset = 0;
        length -= blockLength;
        free(cmpBuf);
    }
    free(decBuf);
    free(offsets);
    return written_bytes;
}

int lz41_stream_compress( FILE* file_in, FILE* file_out )
{
    LZ4_stream_t lz4Stream_body;
    LZ4_stream_t* lz4Stream = &lz4Stream_body;
    int written_bytes = 0;

    char inpBuf[LZ41_BLOCK_BYTES];
    fseek(file_in, 0, SEEK_END);
    int file_size = ftell(file_in);
    int max_blocks = file_size + sizeof(LZ41_FILE_HEADER) / LZ41_BLOCK_BYTES;
    int* offsets = (int*)malloc(max_blocks);
    int* offsetsEnd = offsets;
    fseek(file_in, 0, SEEK_SET);

    LZ4_initStream(lz4Stream, sizeof(*lz4Stream));

    /* Write header */
    fwrite(LZ41_FILE_HEADER, 1, sizeof(LZ41_FILE_HEADER), file_out);
    written_bytes += sizeof(LZ41_FILE_HEADER);
    *offsetsEnd++ = sizeof(LZ41_FILE_HEADER);

    /* Write compressed data blocks.  Each block contains BLOCK_BYTES of plain
       data except possibly the last. */
    for (;;) 
    {
        const int inpBytes = fread(inpBuf, 1, LZ41_BLOCK_BYTES,file_in);
        if (0 == inpBytes) {
            break;
        }

        LZ4_resetStream(lz4Stream);

        char cmpBuf[LZ4_COMPRESSBOUND(LZ41_BLOCK_BYTES)];
        int cmpBytes = LZ4_compress_fast_continue(lz4Stream, inpBuf, cmpBuf, inpBytes, sizeof(cmpBuf), 1);
        if (cmpBytes <= 0) 
           return LZ41_COMPRESSION_ERROR;

        fwrite(cmpBuf, (size_t)cmpBytes, 1, file_out);
        written_bytes += (size_t)cmpBytes;

        /* Keep track of the offsets */
        *offsetsEnd = *(offsetsEnd - 1) + cmpBytes;
        ++offsetsEnd;

        if (offsetsEnd - offsets > max_blocks)
            return LZ41_MAX_BLOCKS_OVERFLOW;
    }

    /* Write the tailing jump table */
    int* ptr = offsets;
    while (ptr != offsetsEnd) 
    {
        fwrite(ptr++, sizeof(int), 1, file_out);
        written_bytes += sizeof(int);
    }

    /* Write number of offsets */
    int i = offsetsEnd - offsets;
    fwrite(&i, sizeof(int), 1, file_out);
    written_bytes += sizeof(int);

    /* Write Filesize */
    fwrite(&file_size, 4, 1, file_out);
    written_bytes += sizeof(int);

    /* Write Block Size */
    int bb = LZ41_BLOCK_BYTES;
    fwrite(&bb, 4, 1, file_out);
    written_bytes += sizeof(int);

    free(offsets);
    return written_bytes;
}

int lz41_stream_compress_HC(FILE* file_in, FILE* file_out,int compression_level)
{
    LZ4_streamHC_t lz4Stream_body;
    LZ4_streamHC_t* lz4Stream = &lz4Stream_body;
    int written_bytes = 0;

    char inpBuf[LZ41_BLOCK_BYTES];
    fseek(file_in, 0, SEEK_END);
    int file_size = ftell(file_in);
    int max_blocks = file_size + sizeof(LZ41_FILE_HEADER) / LZ41_BLOCK_BYTES;
    int* offsets = (int*)malloc(max_blocks);
    int* offsetsEnd = offsets;
    fseek(file_in, 0, SEEK_SET);

    LZ4_initStreamHC(lz4Stream, sizeof(*lz4Stream));

    /* Write header */
    fwrite(LZ41_FILE_HEADER, 1, sizeof(LZ41_FILE_HEADER), file_out);
    written_bytes += sizeof(LZ41_FILE_HEADER);
    *offsetsEnd++ = sizeof(LZ41_FILE_HEADER);

    /* Write compressed data blocks.  Each block contains BLOCK_BYTES of plain
       data except possibly the last. */
    for (;;)
    {
        const int inpBytes = fread(inpBuf, 1, LZ41_BLOCK_BYTES, file_in);
        if (0 == inpBytes) {
            break;
        }

        LZ4_resetStreamHC(lz4Stream,compression_level);

        char cmpBuf[LZ4_COMPRESSBOUND(LZ41_BLOCK_BYTES)];
        int cmpBytes = LZ4_compress_HC_continue(lz4Stream, inpBuf, cmpBuf, inpBytes, sizeof(cmpBuf));
        if (cmpBytes <= 0)
            return LZ41_COMPRESSION_ERROR;

        fwrite(cmpBuf, (size_t)cmpBytes, 1, file_out);
        written_bytes += (size_t)cmpBytes;

        /* Keep track of the offsets */
        *offsetsEnd = *(offsetsEnd - 1) + cmpBytes;
        ++offsetsEnd;

        if (offsetsEnd - offsets > max_blocks)
            return LZ41_MAX_BLOCKS_OVERFLOW;
    }

    /* Write the tailing jump table */
    int* ptr = offsets;
    while (ptr != offsetsEnd)
    {
        fwrite(ptr++, sizeof(int), 1, file_out);
        written_bytes += sizeof(int);
    }

    /* Write number of offsets */
    int i = offsetsEnd - offsets;
    fwrite(&i, sizeof(int), 1, file_out);
    written_bytes += sizeof(int);

    /* Write Filesize */
    fwrite(&file_size, 4, 1, file_out);
    written_bytes += sizeof(int);

    /* Write Block Size */
    int bb = LZ41_BLOCK_BYTES;
    fwrite(&bb, 4, 1, file_out);
    written_bytes += sizeof(int);

    free(offsets);
    return written_bytes;
}

int lz41_stream_decompress(FILE* file_in, FILE* file_out)
{
    LZ4_streamDecode_t lz4StreamDecode_body;
    LZ4_streamDecode_t* lz4StreamDecode = &lz4StreamDecode_body;
    int offset = 0;
    int length = 0;
    int written_bytes = 0;
    int block_size = 0;
    int numOffsets;

    /* Num Offsets */
    fseek(file_in, -12, SEEK_END);
    fread(&numOffsets, sizeof(int), 1, file_in);

    /* File Size */
    fread(&length, sizeof(int), 1, file_in);

    /* Block Size */
    fread(&block_size, sizeof(int), 1, file_in);

    char* decBuf = (char*)malloc(block_size);
    fseek(file_in, 0, SEEK_END);
    int max_blocks = ftell(file_in) + 8 / block_size;
    int* offsets = (int*)malloc(max_blocks);
    fseek(file_in, 0, SEEK_SET);

    /*Read Header*/
    char header[sizeof(LZ41_FILE_HEADER)];
    fread(header, 1, sizeof(LZ41_FILE_HEADER), file_in);
    if (memcmp(LZ41_FILE_HEADER, header, sizeof(LZ41_FILE_HEADER)))
        return LZ41_HEADER_MISMATCH;

    /* The blocks [currentBlock, endBlock) contain the data we want */
    int currentBlock = offset / block_size;
    int endBlock = ((offset + length - 1) / block_size) + 1;

    /* Special cases */
    if (length == 0)
        return 0;

    /* Read the offsets tail */
    int block;
    int* offsetsPtr = offsets;
    if (numOffsets <= endBlock)
        return LZ41_OFFSETS_MISMATCH;
    fseek(file_in, (-4 * (numOffsets + 1)) - (sizeof(int) * 2), SEEK_END);
    for (block = 0; block <= endBlock; ++block)
        fread(offsetsPtr++, sizeof(int), 1, file_in);

    /* Seek to the first block to read */
    fseek(file_in, offsets[currentBlock], SEEK_SET);
    offset = offset % block_size;

    /* Start decoding */
    for (; currentBlock < endBlock; ++currentBlock)
    {
        char* cmpBuf = (char*)malloc(LZ4_compressBound(block_size));
        /* The difference in offsets is the size of the block */
        int  cmpBytes = offsets[currentBlock + 1] - offsets[currentBlock];
        fread(cmpBuf, (size_t)cmpBytes, 1, file_in);

        const int decBytes = LZ4_decompress_safe_continue(lz4StreamDecode, cmpBuf, decBuf, cmpBytes, block_size);
        if (decBytes <= 0)
            return LZ41_DECOMPRESSION_ERROR;

        /* Write out the part of the data we care about */
        int blockLength = ((length) < ((decBytes - offset)) ? (length) : ((decBytes - offset)));
        fwrite(decBuf + offset, (size_t)blockLength, 1, file_out);
        written_bytes += blockLength;
        offset = 0;
        length -= blockLength;
        free(cmpBuf);
    }
    free(decBuf);
    free(offsets);
    return written_bytes;
}

int main()
{
    int input;  
    char fn_in[200], fn_out[200];
    FILE* vp_in;
    FILE* vp_out;

    std::cout << "Block Size is: " << LZ41_BLOCK_BYTES<<"\n";

    printf("\n RECOMENDED COMPRESSION: LZ4-HC Level 6\n");

    printf("Enter: \n 1 - To Compress all .vps files in the folder using LZ4.");  
    printf("\n 2 - To Compress all .vps files in the folder using LZ4-HC.");
    printf("\n 3 - To Decompress all .vps files in the folder.");
    printf("\n");
    printf("\n 4 - To Compress a single file using LZ4.");
    printf("\n 5 - To Compress a single file using LZ4-HC.");
    printf("\n 6 - To Decompress a single file.\n");
    printf("\n");
    printf("\n 7 - To Compress a single .VP file using LZ4.");
    printf("\n 8 - To Compress a single .VP file using LZ4-HC.");
    printf("\n 9 - To Decompress a single .VP file.");
    printf("\n");
    printf("\n 10 - Random Access Test.\n");
    scanf("%d", &input);                   
    printf("\n");

    switch (input)
    {
        case 1 : compress_folder(4); break;
        case 2 : printf("\n Enter Compression Level (5 to 12, Default is 9): \n");
                 scanf("%d", &input); 
                 compress_folder(input); 
                 break;
        case 3 : decompress_folder(); break;
        case 4 : 
            printf("\n Enter filename (with ext): \n");
            scanf("%s", &fn_in);
            strcpy(fn_out, "compressed_");
            strcpy(fn_out+11, fn_in);
            compress_single_file_lz4(fn_in, fn_out, 4);
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
            compress_vp(vp_in, vp_out, 4);
            break;
        case 8:
            printf("\n Enter filename (with ext): \n");
            scanf("%s", &fn_in);
            printf("\nCompressing...\n");
            strcpy(fn_out, "compressed_");
            strcpy(fn_out + 11, fn_in);
            vp_in = fopen(fn_in, "rb");
            vp_out = fopen(fn_out, "wb");
            printf("\n Enter Compression Level (5 to 12, Default is 9): \n");
            scanf("%d", &input);
            compress_vp(vp_in, vp_out, input);
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
        case 10:
            int offset=0;
            int length=0;
            int type = 1;
            char* data;
            printf("\n Enter file name (with ext): \n");
            scanf("%s", &fn_in);
            printf("\n Enter offset: \n");
            scanf("%d", &offset);
            printf("\n Enter length: \n");
            scanf("%d", &length);
            printf("\n Data type? 1-int, 2-char/string: \n");
            scanf("%d", &type);
            vp_in = fopen(fn_in, "rb");
            data = (char*)malloc(length+1);
            int result = lz41_stream_read_random_access(vp_in, data, offset, length);
            fclose(vp_in);
            if (result <= 0)
                std::cout << "Error :" << result;
            else
            {
                if (type == 1)
                {
                    std::cout << "DATA :" << (int)*data;
                }
                else
                {
                    data[length] = '\0';
                    std::cout << "DATA :" << data;
                }
            }
            break;
    }
    system("pause");
}

void compress_single_file_lz4(char* fn_in, char* fn_out, int compression_level)
{
    int size, final_size;
    FILE* file_in = fopen(fn_in, "rb");
    FILE* file_out = fopen(fn_out, "wb");

    if (file_in != NULL)
    {
        fseek(file_in, 0, SEEK_END);
        size = ftell(file_in);
        std::cout << "Original Size: " << size << "\n";
        fseek(file_in, 0, SEEK_SET);
        unsigned int max_memory = LZ4_COMPRESSBOUND(size);
        
        if (compression_level <= 4)
            final_size = lz41_stream_compress(file_in, file_out);
        else
            final_size = lz41_stream_compress_HC(file_in, file_out, compression_level);

        std::cout << "Compressed Size: " << final_size << "\n";
        fclose(file_out);
        fclose(file_in);
    }
    else
    {
        std::cout << "\n" << "Unable to open file : " << fn_in << "\n";
    }
}

void decompress_single_file_lz4(char* fn_in, char* fn_out)
{
    int final_size;
    FILE* file_in = fopen(fn_in, "rb");
    FILE* file_out = fopen(fn_out, "wb");
    if (file_in != NULL)
    {
        int final_size=lz41_stream_decompress(file_in, file_out);

        if (final_size>0)
        {
            std::cout << "Final Size:" << final_size<<"\n";        
        }
        else
        {
            std::cout << "\n" << "Unable to open file : " << fn_in << "\n" << "or file header does not match" << "\n";
        }
        fclose(file_in);
        fclose(file_out);
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
            if (!strstr(LZ41_DO_NOT_COMPRESS, file_extension)&&index[x].filesize>=LZ41_MINIMUM_SIZE)
            {
                ubyte* compressed_bytes;
                unsigned int max_memory = LZ4_COMPRESSBOUND(index[x].filesize);
                compressed_bytes = (ubyte*)malloc(max_memory*2); //Doubling due to extra space needed to store offsets

                int newsize;

                if (compression_level <= 4)
                    newsize = lz41_compress_memory(file, compressed_bytes, index[x].filesize);
                else
                    newsize = lz41_compress_memory_HC(file, compressed_bytes, index[x].filesize, compression_level);

                if (newsize <= 0)
                    std::cout << "COMPRESSION ERROR: " << newsize << " File:" << index[x].name;
                
                /*Do not write compressed files that are larger than the original, also skip errors*/
                if (newsize >= index[x].filesize || newsize <= 0)
                {
                    if(newsize <= 0)
                        std::cout << "\nSkipping " << index[x].name << " compression error. Result: " << newsize;
                    else
                        std::cout << "\nSkipping " << index[x].name << " compressed is larger than the original C: " << newsize << " | O: " << index[x].filesize;
                    write_vp_file(vp_out, file, index[x].name, index[x].filesize, getUnixTime(), index_out, &wvp_num_files, &wvp_index_offset);
                }
                else
                {
                    write_vp_file(vp_out, compressed_bytes, index[x].name, newsize, getUnixTime(), index_out, &wvp_num_files, &wvp_index_offset);
                }
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
            memcpy(&original_size, file+(index[x].filesize-8), sizeof(int));
            
            if (strcmp(file_header, LZ41_FILE_HEADER) == 0)
            {
                uncompressed_bytes = (ubyte*)malloc(original_size);
                int result=lz41_decompress_memory(file, uncompressed_bytes, index[x].filesize, original_size);
                if (result <= 0)
                    std::cout << "DECOMPRESSION ERROR: " << result << " File:" << index[x].name;
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