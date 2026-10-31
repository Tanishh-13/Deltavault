
#include "deltavault/object_store.hpp"
#include <fstream>
#include <stdexcept>
#include <system_error>

namespace deltavault {

ObjectStore::ObjectStore(const std::filesystem::path& vault_path)
    : objects_path_(vault_path / "objects") {
    std::error_code error;
    std::filesystem::create_directories(objects_path_, error);
    if (error) {
        throw std::runtime_error("Failed to create objects directory: " + error.message());
    }
}

std::filesystem::path ObjectStore::object_path(const std::string& object_id) const {
    if (object_id.empty()) {
        throw std::invalid_argument("Object ID cannot be empty");
    }

    for (unsigned char c : object_id) {
        if (!((c >= 'a' && c <= 'z') ||
              (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') ||
              c == '-' || c == '_')) {
            throw std::invalid_argument("Invalid object ID");
        }
    }

    return objects_path_ / object_id;
}

bool ObjectStore::contains(const std::string& object_id) const {
    const auto path = object_path(object_id);
    std::error_code error;
    const bool exists = std::filesystem::is_regular_file(path, error);

    if (error == std::errc::no_such_file_or_directory) {
        return false;
    }

    if (error) {
        throw std::runtime_error("Failed to check object: " + error.message());
    }

    return exists;
}

void ObjectStore::put(const std::string& object_id, const Bytes& data) {
    const auto path = object_path(object_id);

    if (contains(object_id)) {
        return;
    }

    std::ofstream output(path, std::ios::binary | std::ios::out);
    if (!output) {
        throw std::runtime_error("Failed to create object: " + object_id);
    }

    if (!data.empty()) {
        output.write(reinterpret_cast<const char*>(data.data()),
                     static_cast<std::streamsize>(data.size()));
    }

    output.close();
    if (!output) {
        throw std::runtime_error("Failed to write object: " + object_id);
    }
}

Bytes ObjectStore::get(const std::string& object_id) const {
    const auto path = object_path(object_id);
    std::ifstream input(path, std::ios::binary | std::ios::ate);

    if (!input) {
        throw std::runtime_error("Failed to open object: " + object_id);
    }

    const std::ifstream::pos_type end = input.tellg();
    if (end < 0) {
        throw std::runtime_error("Failed to determine object size: " + object_id);
    }

    const auto size = static_cast<std::uintmax_t>(end);
    if (size > std::vector<std::uint8_t>().max_size()) {
        throw std::runtime_error("Object is too large: " + object_id);
    }

    Bytes data(static_cast<std::size_t>(size));
    input.seekg(0, std::ios::beg);

    if (!data.empty()) {
        input.read(reinterpret_cast<char*>(data.data()),
                   static_cast<std::streamsize>(data.size()));

        if (!input || input.gcount() != static_cast<std::streamsize>(data.size())) {
            throw std::runtime_error("Failed to read object: " + object_id);
        }
    }

    return data;
}

} 
