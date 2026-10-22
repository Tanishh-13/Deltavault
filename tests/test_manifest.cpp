
#include <cassert>
#include <iostream>
#include "deltavault/manifest.hpp"

int main() {
    std::vector<std::string> hashes = {
        "hash_A",
        "hash_B",
        "hash_C"
    };

    deltavault::Manifest manifest =
        deltavault::create_manifest("movie.mp4", hashes);

    assert(manifest.filename == "movie.mp4");
    assert(manifest.chunk_hashes.size() == 3);
    assert(manifest.chunk_hashes[0] == "hash_A");
    assert(manifest.chunk_hashes[1] == "hash_B");
    assert(manifest.chunk_hashes[2] == "hash_C");

    std::cout << "All manifest tests passed!\n";

    return 0;
}
