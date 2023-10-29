import png
import random
import sys


def unique_random_pixel_generator(cnt_rows, cnt_columns):
    used = set()

    def _next():
        while True:
            row = random.randint(0, cnt_rows - 1)
            column = random.randint(0, cnt_columns - 1)
            pair = (row, column)
            if pair not in used and column % 4 == 1:
                used.add(pair)
                return pair

    return _next


def read_file_as_bytes(filename):
    with open(filename, 'rb') as file_:
        return list(file_.read())


def bytes_to_bits(bytes):
    bits = []
    for byte in bytes:
        bits += [((1 << i) & byte) >> i for i in range(8)]
    return bits


def bits_to_int(bits):
    res = 0
    for i, bit in enumerate(bits):
        res |= bit << i
    return res


def bits_to_bytes(bits):
    cnt = len(bits) // 8
    return [bits_to_int(bits[i * 8: (i + 1) * 8]) for i in range(cnt)]


def int32_to_bits(n):
    return [((1 << i) & n) >> i for i in range(32)]


def write_bits_to_pixels(pixels, gen, bits):
    for b in bits:
        r, c = gen()
        pixels[r][c] = (pixels[r][c] & 0xFE) | b


def read_bits_from_pixels(pixels, gen, count):
    bits = []
    for i in range(count):
        r, c = gen()
        bits += [pixels[r][c] & 0x1]
    return bits


def encode(datafile, png_in, png_out, seed=1337):
    data_bits = bytes_to_bits(read_file_as_bytes(datafile))
    size_bytes = int32_to_bits(len(data_bits) // 8)

    with open(png_in, 'rb') as f:
        reader = png.Reader(file=f)
        width, height, pixels, metadata = reader.asDirect()
        pixel_data = [list(row) for row in pixels]

    random.seed(seed)
    gen = unique_random_pixel_generator(height, width * 4)

    write_bits_to_pixels(pixel_data, gen, size_bytes)
    write_bits_to_pixels(pixel_data, gen, data_bits)

    with open(png_out, 'wb') as f:
        writer = png.Writer(width=width, height=height, **metadata)
        writer.write(f, pixel_data)


def decode(png_in, datafile, seed=1337):
    with open(png_in, 'rb') as f:
        reader = png.Reader(file=f)
        width, height, pixels, metadata = reader.asDirect()
        pixel_data = [list(row) for row in pixels]

    random.seed(seed)
    gen = unique_random_pixel_generator(height, width * 4)
    cnt_bytes = bits_to_int(read_bits_from_pixels(pixel_data, gen, 32))
    bits = read_bits_from_pixels(pixel_data, gen, cnt_bytes * 8)
    bytes_ = bits_to_bytes(bits)
    with open(datafile, 'wb') as file:
        file.write(bytes(bytes_))


if sys.argv[1] == "e":
    datafile, png_in, container = sys.argv[2], sys.argv[3], sys.argv[4]
    encode(datafile, png_in, container)

if sys.argv[1] == "d":
    container, png_out = sys.argv[2], sys.argv[3]
    decode(container, png_out)
