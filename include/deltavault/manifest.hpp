// A manifest is a record of the chunks that make up a file, including their order and identifiers.

#ifndef DELTAVAULT_MANIFEST_HPP
#define DELTAVAULT_MANIFEST_HPP
#include "deltavault/chunker.hpp"
#include <string>
#include <vector>

namespace deltavault {

struct Manifest {
    std::string filename;
    std::vector<std::string> chunk_hashes;
};
Manifest create_manifest(
    const std::string& filename,
    const std::vector<std::string>& chunk_hashes
);
Manifest build_manifest(
    const std::string& filename,
    const Chunk& file_data
);

} // namespace deltavault

#endif
