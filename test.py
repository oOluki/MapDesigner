import os
import platform
import sys
import locale

ENCODING = locale.getpreferredencoding()
print(f"Default encoding: {ENCODING}")

if(len(sys.argv) != 5):
    print(f"test expects 4 arguments: path to executable, input, expected output and expected map, got {len(sys.argv) - 1} instead")
    exit(1)

EXECUTABLE = sys.argv[1]
INPUT = sys.argv[2]
OUTPUT = sys.argv[3]
MAP    = sys.argv[4]


if platform.system() == "Windows":
    EXECUTABLE = EXECUTABLE.replace("/", "\\")
    INPUT = INPUT.replace("/", "\\")
    OUTPUT = OUTPUT.replace("/", "\\")

def cmpf(f1, f2, mode):
    file1 = open(f1, mode)
    file2 = open(f2, mode)
    status = file1.read() == file2.read()
    file1.close()
    file2.close()
    return status

print(f"CMD: '{EXECUTABLE} < {INPUT} > tmp.txt'")
if(os.system(f"{EXECUTABLE} < {INPUT} > tmp.txt")):
    print("run failed^^^")
    exit(1)

if(not cmpf("tmp.txt", OUTPUT, "rb")):
    print("test does not output as expected")
    exit(1)

if(not cmpf("dummymap.txt", MAP, "rb")):
    print("test does not generate expected map")
    exit(1)

print("test success!")
