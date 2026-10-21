
#include "deltavault/object_store.hpp"
#include <cassert>
#include <filesystem>
#include <iostream>
#include <stdexcept>

using namespace deltavault;

int main() {
    const auto test_path = std::filesystem::temp_directory_path() / "deltavault_object_store_test";
    std::filesystem::remove_all(test_path);

    {
        ObjectStore store(test_path);

        assert(std::filesystem::is_directory(test_path / "objects"));
        assert(!store.contains("text001"));

        Bytes text_data = {'H', 'e', 'l', 'l', 'o'};
        store.put("text001", text_data);
        assert(store.contains("text001"));
        assert(store.get("text001") == text_data);

        Bytes binary_data = {0, 255, 1, 0, 128, 42};
        store.put("binary001", binary_data);
        assert(store.get("binary001") == binary_data);

        Bytes empty_data;
        store.put("empty001", empty_data);
        assert(store.contains("empty001"));
        assert(store.get("empty001").empty());

        bool missing_failed = false;
        try {
            store.get("missing001");
        } catch (const std::runtime_error&) {
            missing_failed = true;
        }
        assert(missing_failed);

        Bytes different_data = {'X', 'Y', 'Z'};
        store.put("text001", different_data);
        assert(store.get("text001") == text_data);

        bool invalid_failed = false;
        try {
            store.put("../outside", text_data);
        } catch (const std::invalid_argument&) {
            invalid_failed = true;
        }
        assert(invalid_failed);

        ObjectStore second_store(test_path);
        assert(second_store.get("text001") == text_data);
        assert(second_store.get("binary001") == binary_data);
    }

    std::filesystem::remove_all(test_path);
    std::cout << "All object store tests passed.\n";
    return 0;
}
