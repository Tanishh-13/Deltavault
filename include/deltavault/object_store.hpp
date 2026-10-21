#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace deltavault {

using Bytes = std::vector<std::uint8_t>;

class ObjectStore {
public:
    explicit ObjectStore(const std::filesystem::path& vault_path);

    bool contains(const std::string& object_id) const;

    void put(const std::string& object_id, const Bytes& data);

    Bytes get(const std::string& object_id) const;

private:
    std::filesystem::path objects_path_;

    std::filesystem::path object_path(
        const std::string& object_id
    ) const;
};

} 