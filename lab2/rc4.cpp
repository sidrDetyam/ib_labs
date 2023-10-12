#include <iostream>
#include <vector>
#include <algorithm>
#include <string>

std::vector<uint8_t> sBlockInit(const std::vector<uint8_t> &key) {
    std::vector<uint8_t> sBlock(256);
    for (uint32_t i = 0; i < 256; ++i) {
        sBlock[i] = i;
    }

    uint8_t j = 0;
    for (uint32_t i = 0; i < 256; ++i) {
        j += sBlock[i] + key[i % key.size()];
        std::swap(sBlock[i], sBlock[j]);
    }

    return sBlock;
}

class Generator {
public:
    explicit Generator(std::vector<uint8_t> block) :
            sBlock(std::move(block)) {}

    uint8_t nextByte() {
        ++i;
        j += sBlock[i];
        std::swap(sBlock[i], sBlock[j]);
        uint8_t t = sBlock[i] + sBlock[j];
        return sBlock[t];
    }

private:
    uint8_t i = 0;
    uint8_t j = 0;
    std::vector<uint8_t> sBlock;
};

std::string encrypt(const std::string &s, const std::vector<uint8_t> &key) {
    Generator generator (sBlockInit(key));
    std::vector<uint8_t> encrypted;
    for (uint8_t c: s) {
        encrypted.push_back(c ^ generator.nextByte());
    }

    return {encrypted.begin(), encrypted.end()};
}

int main() {
    std::vector<uint8_t> key = {1, 2, 3, 4};
    const std::string plain = "hello ib_lab2";
    auto encrypted = encrypt(plain, key);
    auto decrypted = encrypt(encrypted, key);

    // Проверяем, что мы правильно декодируем строку
    std::cout << (plain == decrypted) << std::endl;

    return 0;
}
