#include "hkdf.hpp"
#include "tls_record.hpp"
#include <cassert>
#include <iostream>

void TestHKDFExpandLabel() {
    std::vector<uint8_t> secret(32, 0xAA);
    auto expanded = tls13::HKDF::ExpandLabel(secret, "derived", {}, 32);
    assert(expanded.size() == 32);

    tls13::RecordLayer rec;
    rec.SetKeys(secret, std::vector<uint8_t>(12, 0xBB));
    std::vector<uint8_t> plain = {1, 2, 3, 4};
    auto cipher = rec.EncryptRecord(tls13::ContentType::APPLICATION_DATA, plain);
    assert(cipher.size() > plain.size());

    auto decrypted = rec.DecryptRecord(cipher);
    assert(decrypted.has_value());
    assert(decrypted->type == tls13::ContentType::APPLICATION_DATA);
    assert(decrypted->fragment == plain);

    std::cout << "TestHKDFExpandLabel passed!" << std::endl;
}

int main() {
    TestHKDFExpandLabel();
    return 0;
}
