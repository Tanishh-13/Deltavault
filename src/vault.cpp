#include "deltavault/vault.hpp"
#include <iostream>

namespace deltavault {

Vault::Vault(std::filesystem::path vault_path)
    : vault_path_(std::move(vault_path)),
      objects_dir_(vault_path_ / "objects"),
      manifests_dir_(vault_path_ / "manifests") {}

bool Vault::init() {
    if (vault_path_.empty()) {
        std::cerr << "Error: Vault path cannot be empty.\n";
        return false;
    }

    try {
        if (std::filesystem::exists(vault_path_) && is_valid()) {
            std::cerr << "Error: Vault already initialized at " << vault_path_ << "\n";
            return false;
        }
        if (std::filesystem::exists(vault_path_) && !std::filesystem::is_directory(vault_path_)) {
            std::cerr << "Error: Path exists and is not a directory: " << vault_path_ << "\n";
            return false;
        }

        std::filesystem::create_directories(vault_path_);
        std::filesystem::create_directories(objects_dir_);
        std::filesystem::create_directories(manifests_dir_);
        return is_valid();
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Filesystem error during vault initialization: " << e.what() << "\n";
        return false;
    }
}

bool Vault::is_valid() const {
    return !vault_path_.empty() &&
           std::filesystem::is_directory(vault_path_) &&
           std::filesystem::is_directory(objects_dir_) &&
           std::filesystem::is_directory(manifests_dir_);
}

// Interface stubs for Days 2-5 orchestration
bool Vault::add(const std::filesystem::path& filepath) {
    (void)filepath;
    return false;
}

std::vector<std::string> Vault::list() const {
    return {};
}

std::vector<std::string> Vault::versions(const std::string& filename) const {
    (void)filename;
    return {};
}

bool Vault::restore(const std::string& filename, const std::string& version, const std::filesystem::path& output_path) {
    (void)filename;
    (void)version;
    (void)output_path;
    return false;
}

bool Vault::verify(const std::string& filename, const std::string& version) {
    (void)filename;
    (void)version;
    return false;
}

StatResult Vault::stats() const {
    return {};
}

bool Vault::gc() {
    return false;
}

} // namespace deltavault