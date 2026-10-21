
#include "deltavault/manifest.hpp"
#include <fstream>
#include <limits>
#include <stdexcept>
#include <system_error>

namespace deltavault {

namespace {

bool valid_chunk_id(const std::string& id) {
    if (id.empty()) {
        return false;
    }

    for (unsigned char c : id) {
        if (!((c >= 'a' && c <= 'z') ||
              (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') ||
              c == '-' || c == '_')) {
            return false;
        }
    }

    return true;
}

}

ManifestStore::ManifestStore(const std::filesystem::path& vault_path)
    : manifests_path_(vault_path / "manifests") {
    std::error_code error;
    std::filesystem::create_directories(manifests_path_, error);

    if (error) {
        throw std::runtime_error("Failed to create manifests directory: " + error.message());
    }
}

std::filesystem::path ManifestStore::manifest_path(
    const std::string& logical_filename,
    std::uint64_t version
) const {
    if (logical_filename.empty() || logical_filename == "." || logical_filename == "..") {
        throw std::invalid_argument("Invalid logical filename");
    }

    for (unsigned char c : logical_filename) {
        if (!((c >= 'a' && c <= 'z') ||
              (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') ||
              c == '.' || c == '-' || c == '_')) {
            throw std::invalid_argument("Invalid logical filename");
        }
    }

    if (version == 0) {
        throw std::invalid_argument("Manifest version must be greater than zero");
    }

    return manifests_path_ / (logical_filename + ".v" + std::to_string(version) + ".manifest");
}

void ManifestStore::save(const Manifest& manifest) const {
    const auto path = manifest_path(manifest.logical_filename, manifest.version);

    for (const auto& chunk_id : manifest.chunk_ids) {
        if (!valid_chunk_id(chunk_id)) {
            throw std::invalid_argument("Invalid chunk ID");
        }
    }

    std::ofstream output(path, std::ios::out | std::ios::trunc);

    if (!output) {
        throw std::runtime_error("Failed to create manifest");
    }

    output << manifest.logical_filename << '\n';
    output << manifest.version << '\n';
    output << manifest.chunk_ids.size() << '\n';

    for (const auto& chunk_id : manifest.chunk_ids) {
        output << chunk_id << '\n';
    }

    output.close();

    if (!output) {
        throw std::runtime_error("Failed to write manifest");
    }
}

Manifest ManifestStore::load(
    const std::string& logical_filename,
    std::uint64_t version
) const {
    const auto path = manifest_path(logical_filename, version);
    std::ifstream input(path);

    if (!input) {
        throw std::runtime_error("Failed to open manifest");
    }

    Manifest manifest;
    std::string line;
    std::uint64_t stored_version;
    std::size_t chunk_count;

    if (!std::getline(input, manifest.logical_filename) ||
        !(input >> stored_version) ||
        !(input >> chunk_count)) {
        throw std::runtime_error("Invalid manifest format");
    }

    input.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    if (manifest.logical_filename != logical_filename ||
        stored_version != version ||
        stored_version == 0) {
        throw std::runtime_error("Manifest metadata does not match its path");
    }

    if (chunk_count > manifest.chunk_ids.max_size()) {
        throw std::runtime_error("Invalid manifest chunk count");
    }

    manifest.version = stored_version;
    manifest.chunk_ids.reserve(chunk_count);

    for (std::size_t i = 0; i < chunk_count; ++i) {
        std::string chunk_id;

        if (!std::getline(input, chunk_id) || !valid_chunk_id(chunk_id)) {
            throw std::runtime_error("Invalid chunk ID in manifest");
        }

        manifest.chunk_ids.push_back(chunk_id);
    }

    if (input.peek() != std::char_traits<char>::eof()) {
        throw std::runtime_error("Unexpected data after manifest chunks");
    }

    return manifest;
}

}
