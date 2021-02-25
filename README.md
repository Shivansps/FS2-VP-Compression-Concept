# FS2-VP-Compression-Concept

Using LZ4 lib, nothing too fancy, this code shows how to create compressed vp files, that vp tools can still open and read, but with all the files inside being compressed.
A VP decompression method is also provided, as well as two prebuild bins, a compressor, and a decompressor, these bins will compress/decompress all vp files
that are in the same folder.

Compressed VPs are marked as being "Version 3" in header. 
It is needed to add the original file size to the start of the compressed data as the decompression method needs that data.

Compression is slow, but decompression is very fast. With some files (like the anis or audio files) the result is actually worse.

Compression test:
MVPS 4.4.1:
Original Size: 9.40GB
Compressed Size : 6.10GB


BPComplete 1.1.4:
Original Size: 6.17GB
Compressed Size: 3.21GB
