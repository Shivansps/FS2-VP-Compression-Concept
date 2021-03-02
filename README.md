# FS2-VP-Compression-Concept

Using LZ4 lib, nothing too fancy, this code shows how to create compressed vp files, that vp tools can still open and read, but with all the files inside being compressed.
A VP decompression method is also provided, as well as two prebuild bins, a compressor, and a decompressor, these bins will compress/decompress all vp files
that are in the same folder.

Compression is slow, but decompression is very fast.

The file format is:

4 byte Header + uncompressed_filesize (int) + compressed file data

CLZ41230000333DATA

So the first 4 bytes of the file must be readed, if the header match, (im using "CLZ4") as header for LZ4 compressed files, then the other 4 bytes of data must be read,
thats the original filesize data, and finally the rest, what is passed on to the decompression function.

This allows to have both compressed and uncompressed files whiout having to do any changes to the vp structure, right now, and allows to exclude son files from being compressed,
like very small files or some file that should not be compressed at all, like movies, audios and cbanis.


Compression test:
MVPS 4.4.1:
Original Size: 9.40GB
Compressed Size : 6.10GB


BPComplete 1.1.4:
Original Size: 6.17GB
Compressed Size: 3.21GB
