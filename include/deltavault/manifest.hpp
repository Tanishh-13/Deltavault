// A manifest is a record of the chunks that make up a file, including their order and identifiers.

#ifndef DELTAVAULT_MANIFEST_HPP
#define DELTAVAULT_MANIFEST_HPP


#pragma once

#include <cstdint>
#include <filesystem>
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

} // namespace deltavault

#endif
    std::string logical_filename;
    std::uint64_t version;
    std::vector<std::string> chunk_ids;
};

class ManifestStore {
public:
    explicit ManifestStore(const std::filesystem::path& vault_path);

    void save(const Manifest& manifest) const;
    Manifest load(const std::string& logical_filename, std::uint64_t version) const;

private:
    std::filesystem::path manifests_path_;

    std::filesystem::path manifest_path(
        const std::string& logical_filename,
        std::uint64_t version
    ) const;
};

}
