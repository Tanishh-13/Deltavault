#include "deltavault/vault.hpp"
#include "deltavault/chunker.hpp"
#include "deltavault/hasher.hpp"
#include "deltavault/object_store.hpp"
#include "deltavault/manifest.hpp"

#include <algorithm>
#include <fstream>
#include <iomanip>
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
    if (num_str.empty()) return 0;
    for (unsigned char c : num_str) {
        if (c < '0' || c > '9') return 0;
    }
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
        std::cerr << "Error: Vault is not initialized. Run 'deltavault init' first.\n";
        return false;
    }
    if (!std::filesystem::exists(filepath) || !std::filesystem::is_regular_file(filepath)) {
        std::cerr << "Error: Target file does not exist or is not a regular file: " << filepath << "\n";
        return false;
    }

    std::ifstream file(filepath, std::ios::binary);
    if (!file) {
        std::cerr << "Error: Could not open file for reading: " << filepath << "\n";
        return false;
    }
    std::vector<std::uint8_t> data(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>()
    );

    Chunker chunker;
    std::vector<Chunk> chunks = chunker.split(data);

    std::vector<std::string> chunk_ids;
    chunk_ids.reserve(chunks.size());

    try {
        ObjectStore object_store(vault_path_);
        for (const auto& chunk : chunks) {
            std::string chunk_id = Hasher::sha256(chunk);
            chunk_ids.push_back(chunk_id);

            if (!object_store.contains(chunk_id)) {
                object_store.put(chunk_id, chunk);
            }
        }

        std::string logical_name = filepath.filename().string();
        std::uint64_t next_ver = get_next_version_number(logical_name);

        Manifest manifest{logical_name, next_ver, chunk_ids};
        ManifestStore manifest_store(vault_path_);
        manifest_store.save(manifest);

        std::cout << "Added '" << logical_name << "' as v" << next_ver
                  << " (" << chunks.size() << " chunks, " << data.size() << " bytes)\n";
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error adding file '" << filepath.filename().string() << "': " << e.what() << "\n";
        return false;
    }
}

std::vector<std::string> Vault::list() const {
    if (!is_valid()) {
        return {};
    }

    std::set<std::string> unique_files;
    if (std::filesystem::exists(manifests_dir_)) {
        for (const auto& entry : std::filesystem::directory_iterator(manifests_dir_)) {
            if (entry.is_regular_file() && entry.path().extension() == ".manifest") {
                std::string fname = entry.path().stem().string();
                auto v_pos = fname.rfind(".v");
                if (v_pos != std::string::npos && v_pos > 0) {
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
                std::string stem = entry.path().stem().string();
                auto v_pos = stem.rfind(".v");
                if (v_pos != std::string::npos) {
                    std::string logical = stem.substr(0, v_pos);
                    if (logical == filename) {
                        std::uint64_t ver = parse_version_string(stem.substr(v_pos + 1));
                        if (ver > 0) {
                            version_nums.push_back(ver);
                        }
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
        std::cerr << "Error: Vault is not initialized. Run 'deltavault init' first.\n";
        return false;
    }

    std::uint64_t ver_num = parse_version_string(version);
    if (ver_num == 0) {
        std::cerr << "Error: Invalid version format '" << version << "'. Expected format like 'v1'.\n";
        return false;
    }

    Manifest manifest;
    try {
        ManifestStore manifest_store(vault_path_);
        manifest = manifest_store.load(filename, ver_num);
    } catch (const std::exception& e) {
        std::cerr << "Error: Could not load manifest for '" << filename << "' (" << version << "): " << e.what() << "\n";
        return false;
    }

    ObjectStore object_store(vault_path_);
    std::vector<Bytes> verified_chunks;
    verified_chunks.reserve(manifest.chunk_ids.size());

    // Retrieve and verify integrity of all chunks before writing destination file
    for (const auto& chunk_id : manifest.chunk_ids) {
        try {
            if (!object_store.contains(chunk_id)) {
                std::cerr << "Error: Restore failed — missing chunk object '" << chunk_id << "'\n";
                return false;
            }
            Bytes chunk_data = object_store.get(chunk_id);
            if (Hasher::sha256(chunk_data) != chunk_id) {
                std::cerr << "Error: Restore failed — chunk '" << chunk_id << "' failed SHA-256 integrity check.\n";
                return false;
            }
            verified_chunks.push_back(std::move(chunk_data));
        } catch (const std::exception& e) {
            std::cerr << "Error: Failed to retrieve chunk '" << chunk_id << "': " << e.what() << "\n";
            return false;
        }
    }

    if (output_path.has_parent_path() && !output_path.parent_path().empty()) {
        std::error_code ec;
        std::filesystem::create_directories(output_path.parent_path(), ec);
    }

    std::ofstream out(output_path, std::ios::binary | std::ios::trunc);
    if (!out) {
        std::cerr << "Error: Cannot open destination path for writing: " << output_path << "\n";
        return false;
    }

    for (const auto& chunk_data : verified_chunks) {
        if (!chunk_data.empty()) {
            out.write(reinterpret_cast<const char*>(chunk_data.data()), static_cast<std::streamsize>(chunk_data.size()));
        }
    }

    out.close();
    if (!out) {
        std::cerr << "Error: Failed while writing output file: " << output_path << "\n";
        return false;
    }

    std::cout << "Successfully restored '" << filename << "' (v" << ver_num << ") to " << output_path << "\n";
    return true;
}

bool Vault::verify(const std::string& filename, const std::string& version) {
    if (!is_valid()) {
        std::cerr << "Error: Vault is not initialized. Run 'deltavault init' first.\n";
        return false;
    }

    std::uint64_t ver_num = parse_version_string(version);
    if (ver_num == 0) {
        std::cerr << "Error: Invalid version format '" << version << "'. Expected format like 'v1'.\n";
        return false;
    }

    Manifest manifest;
    try {
        ManifestStore manifest_store(vault_path_);
        manifest = manifest_store.load(filename, ver_num);
    } catch (const std::exception& e) {
        std::cerr << "Error: Verification failed — could not load manifest for '"
                  << filename << "' (" << version << "): " << e.what() << "\n";
        return false;
    }

    ObjectStore object_store(vault_path_);
    for (const auto& chunk_id : manifest.chunk_ids) {
        if (!object_store.contains(chunk_id)) {
            std::cerr << "Verification FAILED: Missing chunk object '" << chunk_id << "'\n";
            return false;
        }

        Bytes chunk_data;
        try {
            chunk_data = object_store.get(chunk_id);
        } catch (const std::exception& e) {
            std::cerr << "Verification FAILED: Unreadable chunk '" << chunk_id << "': " << e.what() << "\n";
            return false;
        }

        std::string computed_hash = Hasher::sha256(chunk_data);
        if (computed_hash != chunk_id) {
            std::cerr << "Verification FAILED: Corrupted chunk '" << chunk_id
                      << "' (expected " << chunk_id << ", got " << computed_hash << ")\n";
            return false;
        }
    }

    std::cout << "Verification PASSED for '" << filename << "' (v" << ver_num
              << "): all " << manifest.chunk_ids.size() << " chunks verified.\n";
    return true;
}

StatResult Vault::stats() const {
    StatResult res{};
    if (!is_valid()) {
        return res;
    }

    ManifestStore manifest_store(vault_path_);

    if (std::filesystem::exists(objects_dir_)) {
        for (const auto& entry : std::filesystem::directory_iterator(objects_dir_)) {
            if (entry.is_regular_file()) {
                res.chunk_count++;
                res.physical_bytes += static_cast<size_t>(std::filesystem::file_size(entry.path()));
            }
        }
    }

    auto files = list();
    for (const auto& file : files) {
        auto vers = versions(file);
        for (const auto& ver_str : vers) {
            std::uint64_t ver_num = parse_version_string(ver_str);
            if (ver_num == 0) continue;
            try {
                Manifest m = manifest_store.load(file, ver_num);
                for (const auto& cid : m.chunk_ids) {
                    std::filesystem::path cpath = objects_dir_ / cid;
                    if (std::filesystem::is_regular_file(cpath)) {
                        res.logical_bytes += static_cast<size_t>(std::filesystem::file_size(cpath));
                    }
                }
            } catch (...) {}
        }
    }

    if (res.logical_bytes > 0 && res.logical_bytes >= res.physical_bytes) {
        res.deduplication_savings =
            (1.0 - (static_cast<double>(res.physical_bytes) / static_cast<double>(res.logical_bytes))) * 100.0;
    } else {
        res.deduplication_savings = 0.0;
    }

    return res;
}

bool Vault::gc() {
    if (!is_valid()) {
        std::cerr << "Error: Vault is not initialized. Run 'deltavault init' first.\n";
        return false;
    }

    ManifestStore manifest_store(vault_path_);
    std::set<std::string> reachable_chunks;

    auto files = list();
    for (const auto& file : files) {
        auto vers = versions(file);
        for (const auto& ver_str : vers) {
            std::uint64_t ver_num = parse_version_string(ver_str);
            if (ver_num == 0) continue;
            try {
                Manifest m = manifest_store.load(file, ver_num);
                for (const auto& cid : m.chunk_ids) {
                    reachable_chunks.insert(cid);
                }
            } catch (const std::exception& e) {
                std::cerr << "Error reading manifest during GC (" << file << " " << ver_str << "): " << e.what() << "\n";
                return false;
            }
        }
    }

    size_t deleted_count = 0;
    size_t reclaimed_bytes = 0;

    if (std::filesystem::exists(objects_dir_)) {
        std::vector<std::filesystem::path> unreferenced;
        for (const auto& entry : std::filesystem::directory_iterator(objects_dir_)) {
            if (entry.is_regular_file()) {
                std::string object_id = entry.path().filename().string();
                if (reachable_chunks.find(object_id) == reachable_chunks.end()) {
                    reclaimed_bytes += static_cast<size_t>(std::filesystem::file_size(entry.path()));
                    unreferenced.push_back(entry.path());
                }
            }
        }

        for (const auto& path : unreferenced) {
            std::error_code ec;
            if (std::filesystem::remove(path, ec)) {
                deleted_count++;
            } else {
                std::cerr << "Error: Failed to remove unreferenced chunk " << path << ": " << ec.message() << "\n";
                return false;
            }
        }
    }

    std::cout << "Garbage Collection complete: removed " << deleted_count
              << " unreferenced chunk(s) (" << reclaimed_bytes << " bytes reclaimed).\n";
    return true;
}

} // namespace deltavault