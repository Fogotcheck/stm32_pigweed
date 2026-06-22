import sys
import zlib

def main():
    if len(sys.argv) != 2:
        sys.exit(1)

    filename = sys.argv[1]
    crc = 0

    try:
        with open(filename, 'rb') as f:
            while chunk := f.read(65536):
                crc = zlib.crc32(chunk, crc)

        print(f"0x{crc & 0xFFFFFFFF:08X}", end='')
        sys.exit(0)
    except Exception:
        sys.exit(1)

if __name__ == "__main__":
    main()
