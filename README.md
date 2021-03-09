# FS2-VP-Compression-Concept

Using LZ4 lib, nothing too fancy, this code shows how to create compressed vp files, that vp tools can still open and read, but with all the files inside being compressed.
A VP decompression method is also provided, as well as a prebuild bin, this bin can compress/decompress all vp files that are in the same folder.

Compression is slow, specially with HC, because implementation is not MT, but decompression is still very fast.

The file format is:

4 byte Header + LZ4 compressed Blocks + offset list + number of offsets + original filesize

HEADER|BLOCKS|OFFSETS|NUMOFFETS|ORIGINAL_FILESIZE

So the first 4 bytes of the file must be readed, if the header match, (im using "LZ41") as header for LZ4 compressed files, then the proccess should start.

This allows to have both compressed and uncompressed files whiout having to do any changes to the vp structure, right now, and allows to exclude son files from being compressed,
like very small files or some file that should not be compressed at all, like movies, audios and cbanis.

Block size must be the same on both compressor and decompressor.

The mayor flaw with LZ4 is that it is providing awful results compressing PNG files, with the result ending almost always bigger than the original file, so it must be skipped.

Compression test:
MVPS 4.4.1 (Full install):
Original Size: 12.90GB


Block Size 8192
Compressed Size (LZ4) : 8.25GB, Time: 23Min
Compressed Size (LZ4-HC-L5) : 7.80GB, Time: 50min
Compressed Size (LZ4-HC-L9) : 7.78GB
