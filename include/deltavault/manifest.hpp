
#ifndef DELTAVAULT_MANIFEST_HPP
#define DELTAVAULT_MANIFEST_HPP

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include "deltavault/chunker.hpp"

namespace deltavault {

struct Manifest {
    std::string filename;
    std::vector<std::string> chunk_hashes;

    // Existing manifest storage fields
    std::string logical_filename;
    std::uint64_t version = 0;
    std::vector<std::string> chunk_ids;
};

Manifest create_manifest(
    const std::string& filename,
    const std::vector<std::string>& chunk_hashes
);

Manifest build_manifest(
    const std::string& filename,
    const Chunk& file_data
);

class ManifestStore {
public:
    explicit ManifestStore(const std::filesystem::path& vault_path);

    void save(const Manifest& manifest) const;

    Manifest load(
        const std::string& logical_filename,
        std::uint64_t version
    ) const;

private:
    std::filesystem::path manifests_path_;

    std::filesystem::path manifest_path(
        const std::string& logical_filename,
        std::uint64_t version
    ) const;
};

} // namespace deltavault

#endif
