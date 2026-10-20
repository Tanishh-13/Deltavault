
#ifndef DELTAVAULT_CHUNKER_HPP
#define DELTAVAULT_CHUNKER_HPP

#include <cstddef>
#include <cstdint>
#include <vector>

namespace deltavault {

using Chunk = std::vector<std::uint8_t>;

class Chunker {
public:
    static constexpr std::size_t MIN_CHUNK_SIZE = 8 * 1024;
    static constexpr std::size_t TARGET_CHUNK_SIZE = 64 * 1024;
    static constexpr std::size_t MAX_CHUNK_SIZE = 128 * 1024;

    std::vector<Chunk> split(const Chunk& data) const;
};

} // namespace deltavault

#endif
