#include "deltavault/vault.hpp"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <set>

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

std::string Vault::generate_next_version(const std::string& filename) const {
    auto vers = versions(filename);
    int max_ver = 0;
    for (const auto& v : vers) {
        if (!v.empty() && v[0] == 'v') {
            try {
                int num = std::stoi(v.substr(1));
                max_ver = std::max(max_ver, num);
            } catch (...) {}
        }
    }
    return "v" + std::to_string(max_ver + 1);
}

bool Vault::add(const std::filesystem::path& filepath) {
    if (!is_valid()) {
        std::cerr << "Error: Vault is not initialized or invalid.\n";
        return false;
    }
    if (!std::filesystem::is_regular_file(filepath)) {
        std::cerr << "Error: Target file does not exist or is not a regular file: " << filepath << "\n";
        return false;
    }

    // 1. Read binary contents of the target file
    std::ifstream file(filepath, std::ios::binary);
    if (!file) {
        std::cerr << "Error: Could not open file for reading: " << filepath << "\n";
        return false;
    }
    std::vector<std::uint8_t> data(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>()
    );

    // 2. Split file into chunks using Chunker (Member A)
    Chunker chunker;
    std::vector<Chunk> chunks = chunker.split(data);

    // 3. Store missing chunks in ObjectStore (Member B)
    ObjectStore store(vault_path_);
    std::vector<std::string> chunk_ids;
    chunk_ids.reserve(chunks.size());

    for (const auto& chunk : chunks) {
        std::string chunk_id = Hasher::sha256(chunk);
        chunk_ids.push_back(chunk_id);

        if (!store.contains(chunk_id)) {
            store.put(chunk_id, chunk);
        }
    }

    // 4. Create and persist version metadata in Manifest (Member B)
    std::string logical_filename = filepath.filename().string();
    std::string version = generate_next_version(logical_filename);

    uint64_t now = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );

    Manifest manifest{
        logical_filename,
        version,
        now,
        static_cast<std::uint64_t>(data.size()),
        chunk_ids
    };

    if (!manifest.save(manifests_dir_)) {
        std::cerr << "Error: Failed to save version manifest.\n";
        return false;
    }

    std::cout << "Added '" << logical_filename << "' as " << version
              << " (" << chunks.size() << " chunks)\n";
    return true;
}

std::vector<std::string> Vault::list() const {
    if (!is_valid()) {
        return {};
    }

    std::set<std::string> unique_files;
    auto manifests = Manifest::load_all(manifests_dir_);
    for (const auto& m : manifests) {
        unique_files.insert(m.filename);
    }

    return std::vector<std::string>(unique_files.begin(), unique_files.end());
}

std::vector<std::string> Vault::versions(const std::string& filename) const {
    if (!is_valid()) {
        return {};
    }

    std::vector<std::string> ver_list;
    auto manifests = Manifest::load_all(manifests_dir_);
    for (const auto& m : manifests) {
        if (m.filename == filename) {
            ver_list.push_back(m.version);
        }
    }
    std::sort(ver_list.begin(), ver_list.end());
    return ver_list;
}

// Stubs for Days 3-5
bool Vault::restore(const std::string& filename, const std::string& version, const std::filesystem::path& output_path) {
    (void)filename; (void)version; (void)output_path;
    return false;
}

bool Vault::verify(const std::string& filename, const std::string& version) {
    (void)filename; (void)version;
    return false;
}

StatResult Vault::stats() const {
    return {};
}

bool Vault::gc() {
    return false;
}

} // namespace deltavault