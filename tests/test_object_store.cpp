#include "deltavault/object_store.hpp"

#include <cassert>
#include <filesystem>
#include <iostream>
#include <stdexcept>

using namespace deltavault;

int main() {
    const auto test_vault =
        std::filesystem::temp_directory_path()
        / "deltavault_object_store_test";

    std::filesystem::remove_all(test_vault);
    try {
        ObjectStore store(test_vault);

        assert(std::filesystem::is_directory(
            test_vault / "objects"
        ));

        Bytes text_bytes = {
            'H', 'e', 'l', 'l', 'o'
        };
        assert(!store.contains("text001"));
        store.put("text001", text_bytes);
        assert(store.contains("text001"));
        assert(store.get("text001") == text_bytes);

        Bytes binary_bytes = {
            0, 255, 1, 0, 128, 42
        };

        store.put("binary001", binary_bytes);

        assert(store.get("binary001") == binary_bytes);

        Bytes empty_bytes;

        store.put("empty001", empty_bytes);

        assert(store.contains("empty001"));
        assert(store.get("empty001").empty());

        bool missing_object_failed = false;

        try {
            store.get("missing001");
        } catch (const std::runtime_error&) {
            missing_object_failed = true;
        }

        assert(missing_object_failed);
        bool duplicate_failed = false;

        try {
            store.put("text001", Bytes{'X'});
        } catch (const std::runtime_error&) {
            duplicate_failed = true;
        }

        assert(duplicate_failed);
        assert(store.get("text001") == text_bytes);
        bool invalid_id_failed = false;

        try {
            store.contains("../outside");
        } catch (const std::invalid_argument&) {
            invalid_id_failed = true;
        }

        assert(invalid_id_failed);
        ObjectStore reopened_store(test_vault);
        assert(reopened_store.get("text001") == text_bytes);
        assert(reopened_store.get("binary001") == binary_bytes);

        std::cout << "All object store tests passed.\n";
    } catch (...) {
        std::filesystem::remove_all(test_vault);
        throw;
    }
    std::filesystem::remove_all(test_vault);
    return 0;
}