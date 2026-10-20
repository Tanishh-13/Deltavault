#include "deltavault/object_store.hpp"

#include <fstream>
#include <stdexcept>
#include <system_error>

namespace deltavault {

ObjectStore::ObjectStore(
    const std::filesystem::path& vault_path
) {
    objects_path_ = vault_path / "objects";

    std::error_code error;

    std::filesystem::create_directories(
        objects_path_,
        error
    );

    if (error) {
        throw std::runtime_error(
            "Failed to create object directory: " +
            error.message()
        );
    }
}

std::filesystem::path ObjectStore::object_path(
    const std::string& object_id
) const {
    if (object_id.empty()) {
        throw std::invalid_argument(
            "Object ID cannot be empty"
        );
    }

    for (unsigned char ch : object_id) {
        const bool allowed =
            (ch >= 'a' && ch <= 'z') ||
            (ch >= 'A' && ch <= 'Z') ||
            (ch >= '0' && ch <= '9') ||
            ch == '-' ||
            ch == '_';

        if (!allowed) {
            throw std::invalid_argument(
                "Object ID contains invalid characters"
            );
        }
    }

    return objects_path_ / object_id;
}

bool ObjectStore::contains(
    const std::string& object_id
) const {
    const auto path = object_path(object_id);
    std::error_code error;

    const bool exists = std::filesystem::is_regular_file(
        path, error
    );
    if (error == std::errc::no_such_file_or_directory) {
        return false;
    }
    if (error) {
        throw std::runtime_error(
            "Failed to check object: " + error.message()
        );
    }
    return exists;
}
void ObjectStore::put(
    const std::string& object_id,
    const Bytes& data
) {
    const auto path = object_path(object_id);

    if (contains(object_id)) {
        throw std::runtime_error(
            "Object already exists: " + object_id
        );
    }

    std::ofstream file(
        path,
        std::ios::binary | std::ios::out
    );

    if (!file) {
        throw std::runtime_error(
            "Failed to open object for writing: " +
            object_id
        );
    }

    if (!data.empty()) {
        file.write(
            reinterpret_cast<const char*>(data.data()),
            static_cast<std::streamsize>(data.size())
        );
    }

    file.close();

    if (!file) {
        throw std::runtime_error(
            "Failed to finish writing object: " +
            object_id
        );
    }
}

Bytes ObjectStore::get(
    const std::string& object_id
) const {
    const auto path = object_path(object_id);

    std::ifstream file(
        path,
        std::ios::binary | std::ios::ate
    );

    if (!file) {
        throw std::runtime_error(
            "Failed to open object for reading: " +
            object_id
        );
    }

    const std::streampos end = file.tellg();

    if (end < 0) {
        throw std::runtime_error(
            "Failed to determine object size: " +
            object_id
        );
    }

    const auto size =
        static_cast<std::uintmax_t>(end);

    if (size > static_cast<std::uintmax_t>(
                   std::vector<std::uint8_t>().max_size())) {
        throw std::runtime_error(
            "Object is too large to load into memory"
        );
    }

    Bytes data(static_cast<std::size_t>(size));

    file.seekg(0, std::ios::beg);

    if (!data.empty()) {
        file.read(
            reinterpret_cast<char*>(data.data()),
            static_cast<std::streamsize>(data.size())
        );
        if (!file || file.gcount() !=
                         static_cast<std::streamsize>(data.size())) {
            throw std::runtime_error(
                "Failed to read complete object: " +
                object_id
            );
        }
    }
    return data;
}
} 