
#include "deltavault/manifest.hpp"
#include "deltavault/object_store.hpp"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

using namespace deltavault;

void require(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error("Test failed: " + message);
    }
}

template <typename Exception, typename Function>
void require_throws(Function function, const std::string& message) {
    try {
        function();
    } catch (const Exception&) {
        return;
    }
    throw std::runtime_error("Expected exception: " + message);
}

int main() {
    const auto test_path = std::filesystem::temp_directory_path() / ("deltavault_manifest_test_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::remove_all(test_path);

    try {
        const Bytes part_a = {'H', 'e', 'l'};
        const Bytes part_b = {'l', 'o', ' '};
        const Bytes part_c = {'W', 'o', 'r', 'l', 'd', '!'};
        const Bytes original = {'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd', '!'};

        {
            ObjectStore objects(test_path);
            ManifestStore manifests(test_path);

            objects.put("chunk_A", part_a);
            objects.put("chunk_B", part_b);
            objects.put("chunk_C", part_c);

            Manifest first_version{"notes.txt", 1, {"chunk_A", "chunk_B", "chunk_C"}};
            manifests.save(first_version);

            Manifest loaded = manifests.load("notes.txt", 1);
            require(loaded.logical_filename == first_version.logical_filename, "filename round-trip");
            require(loaded.version == first_version.version, "version round-trip");
            require(loaded.chunk_ids == first_version.chunk_ids, "chunk order round-trip");

            Bytes restored;
            for (const auto& id : loaded.chunk_ids) {
                const Bytes chunk = objects.get(id);
                restored.insert(restored.end(), chunk.begin(), chunk.end());
            }
            require(restored == original, "reconstructed bytes match original");

            Manifest second_version{"notes.txt", 2, {"chunk_C", "chunk_A"}};
            manifests.save(second_version);
            Manifest loaded_second = manifests.load("notes.txt", 2);
            require(loaded_second.chunk_ids == second_version.chunk_ids, "separate file versions");

            Manifest empty_manifest{"empty.txt", 1, {}};
            manifests.save(empty_manifest);
            require(manifests.load("empty.txt", 1).chunk_ids.empty(), "empty manifest");

            require_throws<std::runtime_error>(
                [&] { manifests.load("missing.txt", 1); },
                "missing manifest"
            );

            require_throws<std::invalid_argument>(
                [&] { manifests.save({"../outside", 1, {"chunk_A"}}); },
                "invalid filename"
            );

            require_throws<std::invalid_argument>(
                [&] { manifests.save({"notes.txt", 0, {"chunk_A"}}); },
                "zero version"
            );

            require_throws<std::invalid_argument>(
                [&] { manifests.save({"invalid.txt", 1, {"../outside"}}); },
                "invalid chunk ID during save"
            );

            require(manifests.load("notes.txt", 1).chunk_ids == first_version.chunk_ids,
                    "invalid save does not overwrite existing manifest");

            const auto manifest_file = test_path / "manifests" / "notes.txt.v1.manifest";

            {
                std::ofstream output(manifest_file, std::ios::trunc);
                output << "notes.txt\n1\n1\n../outside\n";
            }
            require_throws<std::runtime_error>(
                [&] { manifests.load("notes.txt", 1); },
                "invalid chunk ID during load"
            );

            {
                std::ofstream output(manifest_file, std::ios::trunc);
                output << "notes.txt\n1\n1\nchunk_A\nunexpected\n";
            }
            require_throws<std::runtime_error>(
                [&] { manifests.load("notes.txt", 1); },
                "unexpected trailing data"
            );

            {
                std::ofstream output(manifest_file, std::ios::trunc);
                output << "notes.txt\ninvalid\n1\nchunk_A\n";
            }
            require_throws<std::runtime_error>(
                [&] { manifests.load("notes.txt", 1); },
                "malformed version metadata"
            );
        }

        {
            ObjectStore objects(test_path);
            ManifestStore manifests(test_path);
            Manifest loaded = manifests.load("notes.txt", 2);
            Bytes restored;

            for (const auto& id : loaded.chunk_ids) {
                const Bytes chunk = objects.get(id);
                restored.insert(restored.end(), chunk.begin(), chunk.end());
            }

            Bytes expected;
            expected.insert(expected.end(), part_c.begin(), part_c.end());
            expected.insert(expected.end(), part_a.begin(), part_a.end());
            require(restored == expected, "persistence after reopening stores");
        }

        std::filesystem::remove_all(test_path);
        std::cout << "All manifest and integration tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::filesystem::remove_all(test_path);
        std::cerr << error.what() << '\n';
        return 1;
    }
}
