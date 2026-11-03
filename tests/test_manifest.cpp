
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
    const auto test_path = std::filesystem::temp_directory_path() /
        ("deltavault_manifest_test_" +
         std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));

    try {
        std::filesystem::remove_all(test_path);

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

            Manifest first{"notes.txt", 1, {"chunk_A", "chunk_B", "chunk_C"}};
            manifests.save(first);

            Manifest loaded = manifests.load("notes.txt", 1);

            require(loaded.logical_filename == first.logical_filename, "filename round-trip");
            require(loaded.version == first.version, "version round-trip");
            require(loaded.chunk_ids == first.chunk_ids, "chunk order round-trip");

            Bytes restored;

            for (const auto& id : loaded.chunk_ids) {
                const Bytes chunk = objects.get(id);
                restored.insert(restored.end(), chunk.begin(), chunk.end());
            }

            require(restored == original, "reconstructed bytes");

            Manifest second{"notes.txt", 2, {"chunk_C", "chunk_A"}};
            manifests.save(second);

            require(
                manifests.load("notes.txt", 2).chunk_ids == second.chunk_ids,
                "separate versions"
            );

            Manifest empty{"empty.txt", 1, {}};
            manifests.save(empty);

            require(
                manifests.load("empty.txt", 1).chunk_ids.empty(),
                "empty manifest"
            );

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
                [&] { manifests.save({"notes.txt", 1, {"../outside"}}); },
                "invalid chunk ID"
            );

            require(
                manifests.load("notes.txt", 1).chunk_ids == first.chunk_ids,
                "invalid save preserves existing manifest"
            );

            const auto manifest_file =
                test_path / "manifests" / "notes.txt.v1.manifest";

            {
                std::ofstream output(manifest_file, std::ios::trunc);
                require(static_cast<bool>(output), "open manifest for invalid chunk test");
                output << "notes.txt\n1\n1\n../outside\n";
                require(static_cast<bool>(output), "write invalid chunk test data");
            }

            require_throws<std::runtime_error>(
                [&] { manifests.load("notes.txt", 1); },
                "invalid chunk ID during load"
            );

            {
                std::ofstream output(manifest_file, std::ios::trunc);
                require(static_cast<bool>(output), "open manifest for trailing data test");
                output << "notes.txt\n1\n1\nchunk_A\nunexpected\n";
                require(static_cast<bool>(output), "write trailing data test");
            }

            require_throws<std::runtime_error>(
                [&] { manifests.load("notes.txt", 1); },
                "unexpected trailing data"
            );

            {
                std::ofstream output(manifest_file, std::ios::trunc);
                require(static_cast<bool>(output), "open manifest for invalid version test");
                output << "notes.txt\n1garbage\n1\nchunk_A\n";
                require(static_cast<bool>(output), "write invalid version test data");
            }

            require_throws<std::runtime_error>(
                [&] { manifests.load("notes.txt", 1); },
                "malformed version metadata"
            );

            {
                std::ofstream output(manifest_file, std::ios::trunc);
                require(static_cast<bool>(output), "open manifest for invalid count test");
                output << "notes.txt\n1\n1garbage\nchunk_A\n";
                require(static_cast<bool>(output), "write invalid count test data");
            }

            require_throws<std::runtime_error>(
                [&] { manifests.load("notes.txt", 1); },
                "malformed chunk count metadata"
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

            Bytes expected = part_c;
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
