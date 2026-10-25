
#include "deltavault/manifest.hpp"

namespace deltavault {

// Create a manifest for a file
Manifest create_manifest(
    const std::string& filename,
    const std::vector<std::string>& chunk_hashes
) {
    Manifest manifest;
    manifest.filename = filename;
    manifest.chunk_hashes = chunk_hashes;

    return manifest;
}

} // namespace deltavault
