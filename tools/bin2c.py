import sys

if(len(sys.argv) < 3):
    print("Usage:", sys.argv[0], " input.bin output.h")
    sys.exit(1)

fin_name = sys.argv[1]
fout_name = sys.argv[2]

fin = open(fin_name, "rb")

c_string  = "/* This is a generated file */"
c_string += "u8 CompAssets[] = {\n"

counter = 0
b = fin.read(1)
while b != b'':
    c_string += str(int.from_bytes(b, "little")) + ","
    counter += 1
    if counter == 16:
        c_string += "\n"
        counter = 0
    b = fin.read(1)
fin.close()

c_string += "\n};\n\n"

fout = open(fout_name, "w")
fout.write(c_string)
fout.close()

print("[+] C Assets file generated '" + fout_name + "'")
