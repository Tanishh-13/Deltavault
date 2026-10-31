
#include "deltavault/manifest.hpp"
#include "deltavault/hasher.hpp"

namespace deltavault {

Manifest create_manifest(
    const std::string& filename,
    const std::vector<std::string>& chunk_hashes
) {
    Manifest manifest;
    manifest.filename = filename;
    manifest.chunk_hashes = chunk_hashes;

    return manifest;
}

Manifest build_manifest(
    const std::string& filename,
    const Chunk& file_data
) {
    Chunker chunker;

    // Step 1: Split the file into chunks
    std::vector<Chunk> chunks = chunker.split(file_data);

    // Step 2: Calculate the hash of every chunk
    std::vector<std::string> hashes;

    for (const auto& chunk : chunks) {
        hashes.push_back(Hasher::sha256(chunk));
    }

    // Step 3: Create and return the manifest
    return create_manifest(filename, hashes);
}

} // namespace deltavault
