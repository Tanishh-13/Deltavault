#include "deltavault/vault.hpp"
#include "deltavault/chunker.hpp"
#include "deltavault/hasher.hpp"
#include "deltavault/object_store.hpp"
#include "deltavault/manifest.hpp"

#include <algorithm>
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

std::uint64_t Vault::parse_version_string(const std::string& version_str) {
    if (version_str.empty()) return 0;
    std::string num_str = (version_str[0] == 'v' || version_str[0] == 'V') 
                          ? version_str.substr(1) 
                          : version_str;
    try {
        return std::stoull(num_str);
    } catch (...) {
        return 0;
    }
}

std::uint64_t Vault::get_next_version_number(const std::string& filename) const {
    auto vers = versions(filename);
    std::uint64_t max_ver = 0;
    for (const auto& v : vers) {
        std::uint64_t num = parse_version_string(v);
        max_ver = std::max(max_ver, num);
    }
    return max_ver + 1;
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

    // 1. Read binary contents of the input file
    std::ifstream file(filepath, std::ios::binary);
    if (!file) {
        std::cerr << "Error: Could not open file: " << filepath << "\n";
        return false;
    }
    std::vector<std::uint8_t> data(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>()
    );

    // 2. Split file into chunks using Member A's Chunker
    Chunker chunker;
    std::vector<Chunk> chunks = chunker.split(data);

    // 3. Hash with Member A's Hasher and write missing objects via Member B's ObjectStore
    ObjectStore object_store(vault_path_);
    std::vector<std::string> chunk_ids;
    chunk_ids.reserve(chunks.size());

    for (const auto& chunk : chunks) {
        std::string chunk_id = Hasher::sha256(chunk);
        chunk_ids.push_back(chunk_id);

        if (!object_store.contains(chunk_id)) {
            object_store.put(chunk_id, chunk);
        }
    }

    // 4. Save manifest using Member B's ManifestStore
    std::string logical_name = filepath.filename().string();
    std::uint64_t next_ver = get_next_version_number(logical_name);

    Manifest manifest{logical_name, next_ver, chunk_ids};
    try {
        ManifestStore manifest_store(vault_path_);
        manifest_store.save(manifest);
    } catch (const std::exception& e) {
        std::cerr << "Error saving manifest: " << e.what() << "\n";
        return false;
    }

    std::cout << "Added '" << logical_name << "' as v" << next_ver
              << " (" << chunks.size() << " chunks)\n";
    return true;
}

std::vector<std::string> Vault::list() const {
    if (!is_valid()) {
        return {};
    }

    std::set<std::string> unique_files;
    if (std::filesystem::exists(manifests_dir_)) {
        for (const auto& entry : std::filesystem::directory_iterator(manifests_dir_)) {
            if (entry.is_regular_file() && entry.path().extension() == ".manifest") {
                std::string fname = entry.path().stem().string(); // e.g. "file.txt.v1"
                auto v_pos = fname.rfind(".v");
                if (v_pos != std::string::npos) {
                    unique_files.insert(fname.substr(0, v_pos));
                }
            }
        }
    }

    return std::vector<std::string>(unique_files.begin(), unique_files.end());
}

std::vector<std::string> Vault::versions(const std::string& filename) const {
    if (!is_valid()) {
        return {};
    }

    std::vector<std::uint64_t> version_nums;
    if (std::filesystem::exists(manifests_dir_)) {
        for (const auto& entry : std::filesystem::directory_iterator(manifests_dir_)) {
            if (entry.is_regular_file() && entry.path().extension() == ".manifest") {
                std::string stem = entry.path().stem().string(); // e.g. "file.txt.v1"
                auto v_pos = stem.rfind(".v");
                if (v_pos != std::string::npos) {
                    std::string logical = stem.substr(0, v_pos);
                    if (logical == filename) {
                        try {
                            std::uint64_t ver = std::stoull(stem.substr(v_pos + 2));
                            version_nums.push_back(ver);
                        } catch (...) {}
                    }
                }
            }
        }
    }

    std::sort(version_nums.begin(), version_nums.end());

    std::vector<std::string> result;
    result.reserve(version_nums.size());
    for (std::uint64_t v : version_nums) {
        result.push_back("v" + std::to_string(v));
    }
    return result;
}

bool Vault::restore(const std::string& filename, const std::string& version, const std::filesystem::path& output_path) {
    if (!is_valid()) {
        std::cerr << "Error: Vault is not initialized or invalid.\n";
        return false;
    }

    std::uint64_t ver_num = parse_version_string(version);
    if (ver_num == 0) {
        std::cerr << "Error: Invalid version format '" << version << "'. Expected format like 'v1'.\n";
        return false;
    }

    // 1. Load manifest using Member B's ManifestStore
    Manifest manifest;
    try {
        ManifestStore manifest_store(vault_path_);
        manifest = manifest_store.load(filename, ver_num);
    } catch (const std::exception& e) {
        std::cerr << "Error: Could not load manifest for '" << filename << "' (" << version << "): " << e.what() << "\n";
        return false;
    }

    // 2. Ensure destination directory exists
    if (output_path.has_parent_path()) {
        std::filesystem::create_directories(output_path.parent_path());
    }

    // 3. Open output file and write retrieved chunks sequentially
    std::ofstream out(output_path, std::ios::binary | std::ios::trunc);
    if (!out) {
        std::cerr << "Error: Cannot open destination path for writing: " << output_path << "\n";
        return false;
    }

    ObjectStore object_store(vault_path_);
    for (const auto& chunk_id : manifest.chunk_ids) {
        try {
            Bytes chunk_data = object_store.get(chunk_id);
            if (!chunk_data.empty()) {
                out.write(reinterpret_cast<const char*>(chunk_data.data()), static_cast<std::streamsize>(chunk_data.size()));
            }
        } catch (const std::exception& e) {
            std::cerr << "Error: Failed to retrieve chunk '" << chunk_id << "': " << e.what() << "\n";
            return false;
        }
    }

    out.close();
    std::cout << "Successfully restored '" << filename << "' (" << version << ") to " << output_path << "\n";
    return true;
}

// Days 4-5 Interface stubs
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