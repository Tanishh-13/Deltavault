
#include <iostream>
#include <chrono>
#include <cstdint>
#include <algorithm>
#include "deltavault/chunker.hpp"
#include "deltavault/hasher.hpp"

int main() {
    // Generate 10 MB of sample data
    deltavault::Chunk data(10 * 1024 * 1024);

   std::uint32_t state = 0x12345678;

for (std::size_t i = 0; i < data.size(); ++i) {
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;

    data[i] = static_cast<std::uint8_t>(state & 0xFF);
}

    deltavault::Chunker chunker;

    auto start = std::chrono::steady_clock::now();

    auto chunks = chunker.split(data);
    std::size_t min_size = data.size();
std::size_t max_size = 0;
std::size_t total_size = 0;

for (const auto& chunk : chunks) {
    min_size = std::min(min_size, chunk.size());
    max_size = std::max(max_size, chunk.size());
    total_size += chunk.size();
}

std::cout << "Smallest chunk: " << min_size << " bytes\n";
std::cout << "Largest chunk: " << max_size << " bytes\n";
std::cout << "Average chunk: "
          << total_size / chunks.size() << " bytes\n";

    auto chunk_end = std::chrono::steady_clock::now();

    std::size_t hash_chars = 0;

    for (const auto& chunk : chunks) {
        auto hash = deltavault::Hasher::sha256(chunk);
        hash_chars += hash.size();
    }

    auto hash_end = std::chrono::steady_clock::now();

    auto chunk_time =
        std::chrono::duration<double, std::milli>(
            chunk_end - start).count();

    auto hash_time =
        std::chrono::duration<double, std::milli>(
            hash_end - chunk_end).count();

    std::cout << "Input size: 10 MB\n";
    std::cout << "Chunks generated: " << chunks.size() << "\n";
    std::cout << "Chunking time: " << chunk_time << " ms\n";
    std::cout << "Hashing time: " << hash_time << " ms\n";
    std::cout << "Total time: " << chunk_time + hash_time << " ms\n";
    std::cout << "Hash characters: " << hash_chars << "\n";

    return 0;
}

