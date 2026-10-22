
#include "deltavault/object_store.hpp"
#include <chrono>
#include <filesystem>
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
    const auto test_path = std::filesystem::temp_directory_path() / ("deltavault_object_store_test_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::remove_all(test_path);

    try {
        const Bytes text_data = {'H', 'e', 'l', 'l', 'o'};
        const Bytes binary_data = {0, 255, 1, 0, 128, 42};
        const Bytes empty_data;

        {
            ObjectStore store(test_path);

            require(std::filesystem::is_directory(test_path / "objects"), "objects directory created");
            require(!store.contains("text001"), "missing object returns false");

            store.put("text001", text_data);
            require(store.contains("text001"), "stored object exists");
            require(store.get("text001") == text_data, "text bytes round-trip");

            store.put("binary001", binary_data);
            require(store.get("binary001") == binary_data, "binary bytes round-trip");

            store.put("empty001", empty_data);
            require(store.contains("empty001"), "empty object exists");
            require(store.get("empty001").empty(), "empty object round-trip");

            require_throws<std::runtime_error>(
                [&] { store.get("missing001"); },
                "missing object retrieval"
            );

            store.put("text001", Bytes{'X', 'Y', 'Z'});
            require(store.get("text001") == text_data, "duplicate ID preserves original bytes");

            require_throws<std::invalid_argument>(
                [&] { store.put("../outside", text_data); },
                "path traversal ID rejected by put"
            );

            require_throws<std::invalid_argument>(
                [&] { store.contains("../outside"); },
                "path traversal ID rejected by contains"
            );

            require_throws<std::invalid_argument>(
                [&] { store.get("../outside"); },
                "path traversal ID rejected by get"
            );

            require_throws<std::invalid_argument>(
                [&] { store.put("", text_data); },
                "empty object ID rejected"
            );
        }

        {
            ObjectStore store(test_path);
            require(store.get("text001") == text_data, "text persists after reopening");
            require(store.get("binary001") == binary_data, "binary persists after reopening");
            require(store.get("empty001").empty(), "empty object persists after reopening");
        }

        std::filesystem::remove_all(test_path);
        std::cout << "All object store tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::filesystem::remove_all(test_path);
        std::cerr << error.what() << '\n';
        return 1;
    }
}
