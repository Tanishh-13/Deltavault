#ifndef DELTAVAULT_VAULT_HPP
#define DELTAVAULT_VAULT_HPP

#include <string>
#include <vector>
#include <filesystem>
#include <cstdint>

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

    // Day 2: Add and List
    bool add(const std::filesystem::path& filepath);
    std::vector<std::string> list() const;

    // Day 3: Versions and Restore
    std::vector<std::string> versions(const std::string& filename) const;
    bool restore(const std::string& filename, const std::string& version, const std::filesystem::path& output_path);

    // Days 4-5: Interface stubs
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

    std::uint64_t get_next_version_number(const std::string& filename) const;
    static std::uint64_t parse_version_string(const std::string& version_str);
};

} // namespace deltavault

#endif // DELTAVAULT_VAULT_HPP