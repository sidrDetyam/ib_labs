#include <iostream>
#include <vector>
#include <algorithm>
#include <string>

std::vector<int> sBlockInit(const std::vector<int> &key) {
    std::vector<int> sBlock(256);
    for (int i = 0; i < 256; ++i) {
        sBlock[i] = i;
    }

    int j = 0;
    for (int i = 0; i < 256; ++i) {
        j = (j + sBlock[i] + key[i % key.size()]) % 256;
        std::swap(sBlock[i], sBlock[j]);
    }

    return sBlock;
}

struct Generator {
    int i = 0;
    int j = 0;
    std::vector<int> sBlock;

    char nextByte() {
        i = (i + 1) % 256;
        j = (j + sBlock[i]) % 256;
        std::swap(sBlock[i], sBlock[j]);
        int t = (sBlock[i] + sBlock[j]) % 256;
        return (char) sBlock[t];
    }
};

std::string encrypt(const std::string &s, const std::vector<int> &key) {
    Generator generator;
    generator.sBlock = sBlockInit(key);
    std::vector<char> encrypted;
    for (char c: s) {
        encrypted.push_back(char(c ^ generator.nextByte()));
    }

    return {encrypted.begin(), encrypted.end()};
}

int main() {
    std::vector<int> key = {1, 2, 3, 4};
    const std::string plain = "hello ib_lab2";
    auto encrypted = encrypt(plain, key);
    auto decrypted = encrypt(encrypted, key);
    std::cout << (plain == decrypted) << std::endl;

    return 0;
}
