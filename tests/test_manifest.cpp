
#include <cassert>
#include <iostream>

#include "deltavault/manifest.hpp"
#include "deltavault/hasher.hpp"

int main() {
    // Create sample file data of 200 KB
    deltavault::Chunk file_data(200000);

    for (std::size_t i = 0; i < file_data.size(); ++i) {
        file_data[i] = static_cast<std::uint8_t>(i % 256);
    }

    // Build the manifest automatically
    deltavault::Manifest manifest =
        deltavault::build_manifest("sample.bin", file_data);

    // Verify filename
    assert(manifest.filename == "sample.bin");

    // Verify chunk count and hashes
    deltavault::Chunker chunker;
    auto chunks = chunker.split(file_data);

    assert(manifest.chunk_hashes.size() == chunks.size());

    for (std::size_t i = 0; i < chunks.size(); ++i) {
        assert(
            manifest.chunk_hashes[i] ==
            deltavault::Hasher::sha256(chunks[i])
        );
    }

    std::cout << "Integration test passed!\n";
    std::cout << "Filename: " << manifest.filename << "\n";
    std::cout << "Chunks processed: "
              << manifest.chunk_hashes.size() << "\n";

    return 0;
}
