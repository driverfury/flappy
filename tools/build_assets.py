from PIL import Image
import numpy as np
import math
import sys
import json

if(len(sys.argv) < 3):
    print("Usage:", sys.argv[0], " input.json output.bin")
    sys.exit(1)

fin_name = sys.argv[1]
fout_name = sys.argv[2]

fin = open(fin_name, "r")
json_contents = fin.read()
fin.close()

resources_to_build = json.loads(json_contents)
print(type(resources_to_build))
print(resources_to_build)

def img2pal(im):
    palette = []
    indexes = []
    width, height = im.size
    p = np.array(im)
    for row in p:
        for pixel in row:
            a = 0xff
            r = pixel[0]
            g = pixel[1]
            b = pixel[2]
            argb = (a<<24)|(r<<16)|(g<<8)|(b<<0)
            argb = int(argb)
            if argb not in palette:
                palette.append(argb)
            index = palette.index(argb)
            indexes.append(index)
    return (width, height, palette, indexes)

def fwriteu8(f, val):
    f.write(val.to_bytes(1, "little"))

def fwriteu32(f, val):
    f.write(val.to_bytes(4, "little"))

fname = ""

bin_output_file_name = fout_name

tot_size = 0
f = open(bin_output_file_name, "wb")
for resource in resources_to_build:
    im = Image.open(resource["file_name"])
    (width, height, palette, indexes) = img2pal(im)

    # check that len(palette) < 256
    if len(palette) > 256:
        print("Palette too big (" + str(len(palette)) + " colors)")
        sys.exit(1)

    for c in resource["name"]:
        fwriteu8(f, ord(c))
    fwriteu8(f, 0) # null terminator

    # padding
    padding = 0
    full_len = len(resource["name"]) + 1
    padded_name_len = full_len
    if full_len % 4 != 0:
        padded_len = (math.floor(full_len / 4) + 1) * 4
        padded_name_len = padded_len
        padding = int(padded_len - full_len)
    for i in range(padding):
        fwriteu8(f, 0)

    fwriteu32(f, width)
    fwriteu32(f, height)

    fwriteu32(f, len(palette))
    for color in palette:
        fwriteu32(f, color)

    for index in indexes:
        fwriteu8(f, index)

    # padding
    padding = 0
    full_len = len(indexes)
    padded_indexes = full_len
    if full_len % 4 != 0:
        padded_len = (math.floor(full_len / 4) + 1) * 4
        padded_indexes = padded_len
        padding = int(padded_len - full_len)
    for i in range(padding):
        fwriteu8(f, 0)

    asset_size = padded_name_len + 2*4 + 1*4 + len(palette)*4 + padded_indexes*1
    tot_size += asset_size
    print(resource["name"], asset_size, " Bytes")

print(bin_output_file_name, tot_size, " Bytes")

f.close()

print("[+] Assets file generated '" + bin_output_file_name + "' (" + str(tot_size) + " Bytes)")
