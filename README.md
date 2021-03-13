# FS2-VP-Compression-Concept

Using LZ4 lib, nothing too fancy, this code shows how to create compressed vp files, that vp tools can still open and read, but with all the files inside being compressed.
A VP decompression method is also provided, as well as a prebuild bin, this bin can compress/decompress all vp files that are in the same folder.

Compression is slow, specially with HC, because implementation is not MT, but decompression is still very fast.

The file format is:

(char x4)............(N ints)........(int)..............(int)..............(int) 
Header + N Blocks + offset list + number of offsets + original filesize + block_size

HEADER|BLOCKS|OFFSETS|NUMOFFETS|ORIGINAL_FILESIZE|BLOCK_SIZE

So the first 4 bytes of the file must be readed, if the header match, (im using "LZ41") as header for LZ4 compressed files, then the proccess should start.

This allows to have both compressed and uncompressed files whiout having to do any changes to the vp structure, right now, and allows to exclude son files from being compressed,
like very small files or some file that should not be compressed at all, like movies, audios and cbanis.

The mayor flaw with LZ4 is that it is providing awful results compressing PNG files, with the result ending almost always bigger than the original file, so it must be skipped.
<br/>

Compressed MVP 4.4.1 Block Size: 8192<br/>
1:55 - Mission load time with all models<br/>
4.63GB - Memory used after mission load<br/>
8.25GB - Space used on disk<br/>
<br/>
Uncompressed MVP 4.4.1<br/>
2:31 - Mission load time with all models<br/>
4.45GB - Memory used after mission load<br/>
12.9GB - Space used on Disk<br/>