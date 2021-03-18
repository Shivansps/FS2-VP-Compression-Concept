# FS2-VP-Compression-Concept

Using LZ4 lib, nothing too fancy, this code shows how to create compressed vp files, that vp tools can still open and read, but with all the files inside being compressed.
A VP decompression method is also provided, as well as a prebuild bin, this bin can compress/decompress all vp files that are in the same folder.<br/>

Compression is slow, specially with HC, because implementation is not MT, but decompression is still very fast.<br/>

The file format is:<br/>

(char x4)............(N ints)........(int)..............(int)..............(int) <br/>
Header + N Blocks + offset list + number of offsets + original filesize + block_size<br/>

HEADER|BLOCKS|OFFSETS|NUMOFFETS|ORIGINAL_FILESIZE|BLOCK_SIZE<br/>

So the first 4 bytes of the file must be readed, if the header match, (im using "LZ41") as header for LZ4 compressed files, then the proccess should start.<br/>

This allows to have both compressed and uncompressed files whiout having to do any changes to the vp structure.<br/>

The mayor flaw with LZ4 is that it is providing awful results compressing PNG files, with the result ending almost always bigger than the original file, so it must be skipped.<br/>
<br/>

MVP 4.4.1 - All Models mission load time<br/>
----------------------------------------<br/>
LZ4-HC L6, 7.01GB, BS: 65536, CP TIME: 5m<br/>

0:50 - NVME Uncompressed<br/>
0:50 - NVME Compressed<br/>
1:01 - SSD Compressed<br/>
1:02 - SSD Uncompressed<br/>
1:49 - HDD Compressed<br/>
2:30 - HDD Uncompressed<br/>
<br/>
<br/>
Compressed MVP 4.4.1 Block Size: 65536 LZ4-HC L6<br/>
7.01GB - Space used on disk<br/>
Compression time: 5 minutes<br/>
<br/>
Compressed MVP 4.4.1 Block Size: 65536 LZ4-HC L12 (MAX)<br/>
1:45 - Mission load time with all models<br/>
4.66GB - Memory used after mission load<br/>
6.98GB - Space used on disk<br/>
Compression time: 15 minutes<br/>

Compressed MVP 4.4.1 Block Size: 65536<br/>
1:49 - Mission load time with all models<br/>
4.63GB - Memory used after mission load<br/>
7.75GB - Space used on disk<br/>
Compression time: 4 minutes<br/>

Compressed MVP 4.4.1 Block Size: 16384<br/>
1:55 - Mission load time with all models<br/>
4.53GB - Memory used after mission load<br/>
8.02GB - Space used on disk<br/>
<br/>
Uncompressed MVP 4.4.1<br/>
2:31 - Mission load time with all models<br/>
4.45GB - Memory used after mission load<br/>
12.9GB - Space used on Disk<br/>