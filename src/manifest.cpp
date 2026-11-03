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

    Manifest manifest{};
    std::string stored_filename;
    std::string version_line;
    std::string count_line;

    if (!std::getline(input, stored_filename) ||
        !std::getline(input, version_line) ||
        !std::getline(input, count_line)) {
        throw std::runtime_error("Invalid manifest format");
    }

    auto parse_number = [](const std::string& line) -> std::uint64_t {
        if (line.empty()) {
            throw std::runtime_error("Invalid numeric metadata");
        }

        for (unsigned char c : line) {
            if (c < '0' || c > '9') {
                throw std::runtime_error("Invalid numeric metadata");
            }
        }

        try {
            std::size_t parsed = 0;
            const auto value = std::stoull(line, &parsed);

            if (parsed != line.size()) {
                throw std::runtime_error("Invalid numeric metadata");
            }

            return value;
        } catch (const std::exception&) {
            throw std::runtime_error("Invalid numeric metadata");
        }
    };

    const std::uint64_t stored_version = parse_number(version_line);
    const std::uint64_t chunk_count_value = parse_number(count_line);

    if (stored_filename != logical_filename || stored_version != version) {
        throw std::runtime_error("Manifest metadata does not match its path");
    }

    if (chunk_count_value > manifest.chunk_ids.max_size()) {
        throw std::runtime_error("Invalid manifest chunk count");
    }

    const std::size_t chunk_count = static_cast<std::size_t>(chunk_count_value);

    manifest.logical_filename = stored_filename;
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