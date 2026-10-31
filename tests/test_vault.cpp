#include "deltavault/vault.hpp"
#include <cassert>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <vector>

void write_file(const std::filesystem::path& path, const std::string& content) {
    std::ofstream ofs(path, std::ios::binary | std::ios::trunc);
    ofs << content;
}

std::string read_file(const std::filesystem::path& path) {
    std::ifstream ifs(path, std::ios::binary);
    return std::string((std::istreambuf_iterator<char>(ifs)),
                        std::istreambuf_iterator<char>());
}

void test_vault_init() {
    std::filesystem::path test_dir = "./tmp_test_vault";
    if (std::filesystem::exists(test_dir)) {
        std::filesystem::remove_all(test_dir);
    }

    deltavault::Vault vault(test_dir);
    assert(!vault.is_valid());
    assert(vault.init());
    assert(vault.is_valid());
    assert(std::filesystem::exists(test_dir / "objects"));
    assert(std::filesystem::exists(test_dir / "manifests"));

    std::filesystem::remove_all(test_dir);
    std::cout << "[PASS] test_vault_init\n";
}

void test_vault_day3_restore_and_versions() {
    std::filesystem::path test_dir = "./tmp_vault_day3";
    if (std::filesystem::exists(test_dir)) {
        std::filesystem::remove_all(test_dir);
    }

    deltavault::Vault vault(test_dir);
    assert(vault.init());

    std::filesystem::path file_path = "./doc.txt";
    std::string content_v1 = "Hello DeltaVault Version 1 - Baseline content.\n";
    std::string content_v2 = "Hello DeltaVault Version 2 - Appended modification.\n";

    // 1. Add version 1
    write_file(file_path, content_v1);
    assert(vault.add(file_path));

    // 2. Add version 2
    write_file(file_path, content_v2);
    assert(vault.add(file_path));

    // 3. Inspect versions
    auto vers = vault.versions("doc.txt");
    assert(vers.size() == 2);
    assert(vers[0] == "v1");
    assert(vers[1] == "v2");

    // 4. Test independent byte-for-byte restore of v1 and v2
    std::filesystem::path restore_v1_path = "./doc_restored_v1.txt";
    std::filesystem::path restore_v2_path = "./doc_restored_v2.txt";

    assert(vault.restore("doc.txt", "v1", restore_v1_path));
    assert(vault.restore("doc.txt", "v2", restore_v2_path));

    assert(read_file(restore_v1_path) == content_v1);
    assert(read_file(restore_v2_path) == content_v2);

    // 5. Test nonexistent version fails safely
    assert(!vault.restore("doc.txt", "v999", "./nowhere.txt"));

    // Cleanup
    std::filesystem::remove(file_path);
    std::filesystem::remove(restore_v1_path);
    std::filesystem::remove(restore_v2_path);
    std::filesystem::remove_all(test_dir);
    std::cout << "[PASS] test_vault_day3_restore_and_versions\n";
}

int main() {
    std::cout << "Running Member C Day 3 Integration Tests...\n";
    test_vault_init();
    test_vault_day3_restore_and_versions();
    std::cout << "All Day 3 Integration Tests Passed Successfully!\n";
    return 0;
}