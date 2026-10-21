
#include "deltavault/hasher.hpp"

#include <cassert>
#include <iostream>
#include <string>

using namespace deltavault;

int main() {
    // Test 1: SHA-256 of "abc"
    Chunk data = {'a', 'b', 'c'};

    const std::string expected =
        "ba7816bf8f01cfea414140de5dae2223"
        "b00361a396177a9cb410ff61f20015ad";

    const std::string result = Hasher::sha256(data);

    assert(result == expected);
    std::cout << "Test 1 passed: Known SHA-256 hash\n";

    // Test 2: Identical input must produce identical hashes
    assert(Hasher::sha256(data) == Hasher::sha256(data));
    std::cout << "Test 2 passed: Identical chunks\n";

    // Test 3: Different inputs should produce different hashes
    Chunk other = {'a', 'b', 'd'};
    assert(Hasher::sha256(data) != Hasher::sha256(other));
    std::cout << "Test 3 passed: Different chunks\n";

    // Test 4: Empty input
    Chunk empty;
    assert(Hasher::sha256(empty) ==
        "e3b0c44298fc1c149afbf4c8996fb924"
        "27ae41e4649b934ca495991b7852b855");
    std::cout << "Test 4 passed: Empty input\n";

    
    // Test 5: Larger input
    Chunk large_data(1000, 'a');

    const std::string large_hash = Hasher::sha256(large_data);

    assert(large_hash.size() == 64);
    assert(Hasher::sha256(large_data) == large_hash);

    std::cout << "Test 5 passed: Larger input\n";

    // Test 6: SHA-256 block boundary sizes
    const std::size_t sizes[] = {55, 56, 63, 64, 65};

    for (std::size_t size : sizes) {
        Chunk boundary_data(size, 'a');

        const std::string hash1 = Hasher::sha256(boundary_data);
        const std::string hash2 = Hasher::sha256(boundary_data);

        assert(hash1.size() == 64);
        assert(hash1 == hash2);
    }

    std::cout << "Test 6 passed: SHA-256 block boundaries\n";

    std::cout << "All hasher tests passed!\n";
    return 0;
}
