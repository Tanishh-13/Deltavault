
#ifndef DELTAVAULT_HASHER_HPP
#define DELTAVAULT_HASHER_HPP

#include <string>
#include <cstdint>
#include <vector>

namespace deltavault {

using Chunk = std::vector<std::uint8_t>;

class Hasher {
public:
    static std::string sha256(const Chunk& data);
};

} // namespace deltavault

#endif
