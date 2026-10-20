
#include "deltavault/chunker.hpp"

#include <cassert>
#include <iostream>

using namespace deltavault;

int main() {
    Chunker chunker;

    // Test 1: Empty input
    Chunk empty;
    auto empty_chunks = chunker.split(empty);

    assert(empty_chunks.empty());
    std::cout << "Test 1 passed: Empty input\n";

    // Test 2: Small input
    Chunk data = {'H', 'e', 'l', 'l', 'o'};
    auto chunks = chunker.split(data);

    assert(chunks.size() == 1);
    assert(chunks[0] == data);
    std::cout << "Test 2 passed: Small input preserved\n";

    // Test 3: Reconstruct the original data
    Chunk large_data(200000);

    for (std::size_t i = 0; i < large_data.size(); ++i) {
        large_data[i] = static_cast<std::uint8_t>(i % 256);
    }

    auto large_chunks = chunker.split(large_data);
    Chunk reconstructed;

    for (const auto& chunk : large_chunks) {
        assert(!chunk.empty());
        assert(chunk.size() <= Chunker::MAX_CHUNK_SIZE);
        reconstructed.insert(
            reconstructed.end(),
            chunk.begin(),
            chunk.end()
        );
    }

    assert(reconstructed == large_data);
    std::cout << "Test 3 passed: Original data reconstructed\n";

    // Test 4: Repeatability
    auto repeated_chunks = chunker.split(large_data);

    assert(repeated_chunks == large_chunks);
    std::cout << "Test 4 passed: Repeatable chunk boundaries\n";

    
    // Test 5: Chunk size limits
    for (std::size_t i = 0; i < large_chunks.size(); ++i) {
        const auto size = large_chunks[i].size();
        const bool is_last = (i == large_chunks.size() - 1);

        assert(size <= Chunker::MAX_CHUNK_SIZE);

        if (!is_last) {
            assert(size >= Chunker::MIN_CHUNK_SIZE);
        }
    }

    std::cout << "Test 5 passed: Chunk size limits\n";

    // Test 6: Report actual chunk sizes
    std::size_t total_size = 0;
    std::size_t min_size = large_chunks[0].size();
    std::size_t max_size = 0;

    for (const auto& chunk : large_chunks) {
        total_size += chunk.size();

        if (chunk.size() < min_size) {
            min_size = chunk.size();
        }

        if (chunk.size() > max_size) {
            max_size = chunk.size();
        }
    }

    const double average_size =
        static_cast<double>(total_size) / large_chunks.size();

    std::cout << "Number of chunks: " << large_chunks.size() << '\n';
    std::cout << "Minimum chunk size: " << min_size << " bytes\n";
    std::cout << "Maximum chunk size: " << max_size << " bytes\n";
    std::cout << "Average chunk size: " << average_size << " bytes\n";


    // Test 6: Different input patterns
    for (int pattern = 0; pattern < 3; ++pattern) {
        Chunk input(1024 * 1024);

        for (std::size_t i = 0; i < input.size(); ++i) {
            if (pattern == 0) {
                input[i] = 0;
            } else if (pattern == 1) {
                input[i] = static_cast<std::uint8_t>(i % 256);
            } else {
                input[i] = static_cast<std::uint8_t>((i * 37 + 13) % 256);
            }
        }

        auto result = chunker.split(input);
        Chunk rebuilt;

        for (const auto& chunk : result) {
            assert(!chunk.empty());
            assert(chunk.size() <= Chunker::MAX_CHUNK_SIZE);

            rebuilt.insert(
                rebuilt.end(),
                chunk.begin(),
                chunk.end()
            );
        }

        assert(rebuilt == input);

        std::cout << "Pattern " << pattern + 1
                  << " passed: " << result.size()
                  << " chunks\n";
    }

    std::cout << "All chunker tests passed!\n";
    return 0;
}
