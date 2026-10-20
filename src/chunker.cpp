
#include "deltavault/chunker.hpp"

#include <algorithm>
#include <array>
#include <cstdint>

namespace deltavault {

namespace {

// Fixed Gear table: one value for each possible byte.
std::array<std::uint64_t, 256> make_gear_table()  {
    std::array<std::uint64_t, 256> table{};

    std::uint64_t value = 0x9e3779b97f4a7c15ULL;

    for (std::size_t i = 0; i < table.size(); ++i) {
        value ^= value >> 12;
        value ^= value << 25;
        value ^= value >> 27;
        table[i] = value * 0x2545F4914F6CDD1DULL;
    }

    return table;
}

const auto GEAR = make_gear_table();

} // namespace

std::vector<Chunk> Chunker::split(const Chunk& data) const {
    std::vector<Chunk> chunks;

    if (data.empty()) {
        return chunks;
    }

    std::size_t start = 0;
    std::uint64_t fingerprint = 0;

    // Aim for boundaries around 64 KiB.
    const std::uint64_t BOUNDARY_MASK =
    (1ULL << 10) - 1;

    for (std::size_t i = 0; i < data.size(); ++i) {
        fingerprint =
            (fingerprint << 1) + GEAR[data[i]];

        const std::size_t chunk_size = i - start + 1;

        const bool minimum_reached =
            chunk_size >= MIN_CHUNK_SIZE;

        const bool natural_boundary =
            (fingerprint & BOUNDARY_MASK) == 0;

        const bool maximum_reached =
            chunk_size >= MAX_CHUNK_SIZE;

        if (maximum_reached ||
            (minimum_reached && natural_boundary)) {

            chunks.emplace_back(
                data.begin() + start,
                data.begin() + i + 1
            );

            start = i + 1;
            fingerprint = 0;
        }
    }

    // Include any remaining bytes as the final chunk.
    if (start < data.size()) {
        chunks.emplace_back(
            data.begin() + start,
            data.end()
        );
    }

    return chunks;
}

} // namespace deltavault
