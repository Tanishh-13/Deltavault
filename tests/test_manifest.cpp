
#include "deltavault/manifest.hpp"
#include <cassert>
#include <filesystem>
#include <iostream>
#include <stdexcept>

using namespace deltavault;

int main() {
    const auto test_path = std::filesystem::temp_directory_path() / "deltavault_manifest_test";
    std::filesystem::remove_all(test_path);

    {
        ManifestStore store(test_path);

        Manifest original{
            "notes.txt",
            1,
            {"chunk_A", "chunk_B", "chunk_C"}
        };

        store.save(original);

        Manifest loaded = store.load("notes.txt", 1);

        assert(loaded.logical_filename == original.logical_filename);
        assert(loaded.version == original.version);
        assert(loaded.chunk_ids == original.chunk_ids);

        Manifest second_version{
            "notes.txt",
            2,
            {"chunk_C", "chunk_A"}
        };

        store.save(second_version);

        Manifest loaded_second = store.load("notes.txt", 2);

        assert(loaded_second.logical_filename == "notes.txt");
        assert(loaded_second.version == 2);
        assert(loaded_second.chunk_ids == second_version.chunk_ids);

        bool missing_failed = false;
        try {
            store.load("missing.txt", 1);
        } catch (const std::runtime_error&) {
            missing_failed = true;
        }
        assert(missing_failed);

        bool invalid_filename_failed = false;
        try {
            store.save({"../outside", 1, {"chunk_A"}});
        } catch (const std::invalid_argument&) {
            invalid_filename_failed = true;
        }
        assert(invalid_filename_failed);

        bool invalid_version_failed = false;
        try {
            store.save({"notes.txt", 0, {"chunk_A"}});
        } catch (const std::invalid_argument&) {
            invalid_version_failed = true;
        }
        assert(invalid_version_failed);
    }

    std::filesystem::remove_all(test_path);
    std::cout << "All manifest tests passed.\n";
    return 0;
}
