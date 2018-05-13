'''
This script assembles an executable compressed binary image, consisting of a
loader and a compressed binary image.

The source binary image is compressed using zlib, and then byte-padded to a
word boundary. The resulting output file consists of the concatenation of:
- the loader
- one word specifying the length of the compressed data in bytes (this is
  required by the loader to be able to relocate the compressed data)
- the compressed data including the padding
'''


import zlib
import struct

def main(target, source, env):
    assert len(target) == 1 and len(source) == 2
    
    with open(source[0].name, 'rb') as loaderfile:
        loader = loaderfile.read()

    with open(source[1].name, 'rb') as srcimg:
        compressed = zlib.compress(srcimg.read(), 9)
        
    length = len(compressed)
    padding = '\x00' * (3-((length-1) % 4)) # Pad to multiple of 4 bytes

    with open(target[0].name, 'wb') as outfile:
        outfile.write(loader + struct.pack('<I', len(compressed+padding)) + compressed + padding)
        
