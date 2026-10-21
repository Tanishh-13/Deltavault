#ifndef DELTAVAULT_VAULT_HPP
#define DELTAVAULT_VAULT_HPP

#include <string>
#include <vector>
#include <filesystem>
#include "deltavault/chunker.hpp"
#include "deltavault/hasher.hpp"
#include "deltavault/object_store.hpp"
#include "deltavault/manifest.hpp"

namespace deltavault {

struct StatResult {
    size_t logical_bytes{0};
    size_t physical_bytes{0};
    size_t chunk_count{0};
    double deduplication_savings{0.0};
};

class Vault {
public:
    explicit Vault(std::filesystem::path vault_path = ".deltavault");

    // Day 1: Initialization & Validation
    bool init();
    bool is_valid() const;

    // Day 2: Core Orchestration
    bool add(const std::filesystem::path& filepath);
    std::vector<std::string> list() const;

    // Days 3-5: Interface stubs
    std::vector<std::string> versions(const std::string& filename) const;
    bool restore(const std::string& filename, const std::string& version, const std::filesystem::path& output_path);
    bool verify(const std::string& filename, const std::string& version);
    StatResult stats() const;
    bool gc();

    const std::filesystem::path& get_path() const { return vault_path_; }
    const std::filesystem::path& get_objects_dir() const { return objects_dir_; }
    const std::filesystem::path& get_manifests_dir() const { return manifests_dir_; }

private:
    std::filesystem::path vault_path_;
    std::filesystem::path objects_dir_;
    std::filesystem::path manifests_dir_;

    std::string generate_next_version(const std::string& filename) const;
};

} // namespace deltavault

#endif // DELTAVAULT_VAULT_HPP