import zlib
import base64
import struct

# 1. Generate 256 float values (square wave)
floats = [1.0] * 128 + [-1.0] * 128
uncompressed_data = struct.pack(f'{len(floats)}f', *floats)
uncompressed_size = len(uncompressed_data)

# 2. Compress with zlib
compressed_data = zlib.compress(uncompressed_data)
compressed_size = len(compressed_data)

# 3. Base64 encode
b64_data = base64.b64encode(compressed_data).decode('utf-8')

# 4. Output the Skode commands
with open("test_upwave.txt", "w") as f:
    f.write(f"-upwave START {compressed_size} {uncompressed_size}\n")
    # Output in small chunks to test the DATA accumulator
    for i in range(0, len(b64_data), 64):
        f.write(f"-upwave DATA {b64_data[i:i+64]}\n")
    f.write("-upwave COMMIT 999\n")

print("Generated test_upwave.txt")
