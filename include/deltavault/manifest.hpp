#ifndef DELTAVAULT_MANIFEST_HPP
#define DELTAVAULT_MANIFEST_HPP

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace deltavault {

struct Manifest {
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
    std::filesystem::path manifest_path(const std::string& logical_filename, std::uint64_t version) const;
};

}

#endif