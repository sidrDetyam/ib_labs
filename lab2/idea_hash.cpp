#include <iostream>
#include <vector>
#include <algorithm>
#include <string>
#include <array>
#include <optional>
#include <fstream>
#include <iterator>

std::array<uint16_t, 8> splitToSubkeys(std::array<uint32_t, 4> key) {
    std::array<uint16_t, 8> result{};

    for (int i = 0; i < 4; ++i) {
        result[2 * i] = static_cast<uint16_t>(key[i] >> 16);
        result[2 * i + 1] = static_cast<uint16_t>(key[i] & 0xFFFF);
    }

    return result;
}

void cyclicShift(std::array<uint32_t, 4> &key) {
    constexpr int shift = 25;

    uint32_t first = key[0];
    for (int i = 0; i < 4; i++) {
        key[i] = (key[i] << shift) |
                 ((i + 1 == 4 ? first : key[i + 1]) >> (32 - shift));
    }
}

std::vector<std::vector<uint16_t>> transformSubkeys(const std::vector<uint16_t> &subkeys) {
    std::vector<std::vector<uint16_t>> result;

    for (int i = 0; i < 8; ++i) {
        result.emplace_back(subkeys.begin() + i * 6, subkeys.begin() + i * 6 + 6);
    }
    result.emplace_back(subkeys.begin() + 48, subkeys.end());
    return result;
}

std::vector<uint16_t> makeEncryptSubkeys(std::array<uint32_t, 4> key) {
    std::vector<uint16_t> subkeys;

    for (int i = 0; i < 6; ++i) {
        auto subkeys8 = splitToSubkeys(key);
        subkeys.insert(subkeys.end(), subkeys8.begin(), subkeys8.end());
        cyclicShift(key);
    }
    auto subkeys8 = splitToSubkeys(key);
    subkeys.insert(subkeys.end(), subkeys8.begin(), subkeys8.begin() + 4);

    return subkeys;
}

inline uint32_t modInverse(uint32_t a) {
    int32_t m = 0x10001;
    int32_t m0 = m, t, q;
    int32_t x0 = 0, x1 = 1;

    while (a > 1) {
        q = a / m;
        t = m;
        m = a % m;
        a = t;
        t = x0;
        x0 = x1 - q * x0;
        x1 = t;
    }
    if (x1 < 0) {
        x1 += m0;
    }
    return x1;
}

inline uint16_t sumInverse(uint16_t a) {
    return -a;
}

std::vector<std::vector<uint16_t>> makeDecryptSubkeys(std::array<uint32_t, 4> key) {
    auto ek = transformSubkeys(makeEncryptSubkeys(key));

    std::vector<std::vector<uint16_t>> decryptSubkeys(9);
    for (int i = 1; i < 8; ++i) {
        std::vector<uint16_t> dkeys = {(uint16_t) modInverse(ek[8 - i][0]),
                                       sumInverse(ek[8 - i][2]),
                                       sumInverse(ek[8 - i][1]),
                                       (uint16_t) modInverse(ek[8 - i][3]),
                                       ek[7 - i][4],
                                       ek[7 - i][5]};
        decryptSubkeys[i] = std::move(dkeys);
    }
    decryptSubkeys[0] = {(uint16_t) modInverse(ek[8][0]),
                         sumInverse(ek[8][1]),
                         sumInverse(ek[8][2]),
                         (uint16_t) modInverse(ek[8][3]),
                         ek[7][4],
                         ek[7][5]};

    decryptSubkeys[8] = {(uint16_t) modInverse(ek[0][0]),
                         sumInverse(ek[0][1]),
                         sumInverse(ek[0][2]),
                         (uint16_t) modInverse(ek[0][3])};

    return decryptSubkeys;
}

std::array<uint16_t, 4> splitBlock(uint64_t block) {
    return {static_cast<uint16_t>(block >> 48),
            static_cast<uint16_t>((block >> 32) & 0xFFFF),
            static_cast<uint16_t>((block >> 16) & 0xFFFF),
            static_cast<uint16_t>(block & 0xFFFF)};
}

uint64_t joinBlock(const std::array<uint16_t, 4> &block) {
    uint64_t result = 0;
    result |= static_cast<uint64_t>(block[0]) << 48;
    result |= static_cast<uint64_t>(block[1]) << 32;
    result |= static_cast<uint64_t>(block[2]) << 16;
    result |= static_cast<uint64_t>(block[3]);
    return result;
}

uint16_t prod(uint16_t a, uint16_t b) {
    uint64_t aa = a == 0 ? (1 << 16) : a;
    uint64_t bb = b == 0 ? (1 << 16) : b;
    return static_cast<uint16_t>((aa * bb) % ((1 << 16) + 1) % ((1 << 16)));
}

uint64_t encrypt(uint64_t block, const std::vector<std::vector<uint16_t>> &subkeys) {
    auto block16 = splitBlock(block);

    for (int i = 0; i < 8; ++i) {
        const auto &sk = subkeys[i];
        uint16_t a = prod(block16[0], sk[0]);
        uint16_t b = block16[1] + sk[1];
        uint16_t c = block16[2] + sk[2];
        uint16_t d = prod(block16[3], sk[3]);
        uint16_t e = a ^ c;
        uint16_t f = b ^ d;

        block16[0] = a ^ prod(f + prod(e, sk[4]), sk[5]);
        block16[1] = c ^ prod(f + prod(e, sk[4]), sk[5]);
        block16[2] = b ^ (prod(e, sk[4]) + prod(f + prod(e, sk[4]), sk[5]));
        block16[3] = d ^ (prod(e, sk[4]) + prod(f + prod(e, sk[4]), sk[5]));
    }
    const auto &sk = subkeys[8];
    block16[0] = prod(block16[0], sk[0]);
    uint16_t block16_1_cpy = block16[1];
    block16[1] = block16[2] + sk[1];
    block16[2] = block16_1_cpy + sk[2];
    block16[3] = prod(block16[3], sk[3]);

    return joinBlock(block16);
}

using Iterator = std::istream_iterator<uint8_t>;

template<typename T>
std::optional<T> nextBlock(Iterator &current, Iterator &end) {
    T block = 0;
    if (current == end) {
        return std::nullopt;
    }
    for (int i = 0; i < sizeof(T) && current != end; ++i, ++current) {
        block |= (uint64_t(*current)) << (8 * i);
    }
    return {block};
}

uint32_t lower(uint64_t num) {
    return (num << 32) >> 32;
}

uint32_t upper(uint64_t num) {
    return num >> 32;
}

uint64_t fileHash(const std::string &fileName) {
    std::ifstream is(fileName, std::ios::binary);
    is >> std::noskipws;
    Iterator current{is};
    Iterator end{};
    constexpr std::array<uint32_t, 3> seeds = {0x4a7f2e61, 0x9d3f80c5};
    uint64_t result = 0x2b3c5a7d4f9b8a6c;
    while (true) {
        auto nextFileBlock = nextBlock<uint64_t>(current, end);
        if (!nextFileBlock) {
            break;
        }
        auto ek = transformSubkeys(makeEncryptSubkeys({seeds[0],
                                                       lower(result),
                                                       seeds[1],
                                                       upper(result)}));
        result = encrypt(*nextFileBlock, ek) ^ *nextFileBlock;
    }
    return result;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        std::cerr << "Wrong count of args" << std::endl;
        return 1;
    }

    uint64_t hash = fileHash(std::string(argv[1]));
    std::cout << std::hex << hash << std::endl;

    return 0;
}