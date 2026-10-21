#include "deltavault/vault.hpp"
#include <cassert>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <algorithm>

void create_dummy_file(const std::filesystem::path& path, const std::string& content) {
    std::ofstream ofs(path, std::ios::binary);
    ofs << content;
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

void test_vault_add_and_list() {
    std::filesystem::path test_dir = "./tmp_vault_day2";
    if (std::filesystem::exists(test_dir)) {
        std::filesystem::remove_all(test_dir);
    }

    deltavault::Vault vault(test_dir);
    assert(vault.init());

    // Prepare temporary test files
    std::filesystem::path file1 = "./test_file1.txt";
    std::filesystem::path file2 = "./test_file2.txt";
    create_dummy_file(file1, "Alpha Beta Gamma Delta Vault File One Content");
    create_dummy_file(file2, "Alpha Beta Gamma Delta Vault File Two Content");

    // Add first file
    assert(vault.add(file1));
    auto file_list = vault.list();
    assert(file_list.size() == 1);
    assert(file_list[0] == "test_file1.txt");

    // Add second file
    assert(vault.add(file2));
    file_list = vault.list();
    assert(file_list.size() == 2);
    assert(std::find(file_list.begin(), file_list.end(), "test_file1.txt") != file_list.end());
    assert(std::find(file_list.begin(), file_list.end(), "test_file2.txt") != file_list.end());

    // Add another version of file1
    create_dummy_file(file1, "Alpha Beta Gamma Delta Vault File One Updated");
    assert(vault.add(file1));
    file_list = vault.list();
    // list() should still return distinct filenames
    assert(file_list.size() == 2);

    // Cleanup
    std::filesystem::remove(file1);
    std::filesystem::remove(file2);
    std::filesystem::remove_all(test_dir);
    std::cout << "[PASS] test_vault_add_and_list\n";
}

int main() {
    std::cout << "Running Member C Day 2 Integration Tests...\n";
    test_vault_init();
    test_vault_add_and_list();
    std::cout << "All Day 2 Integration Tests Passed Successfully!\n";
    return 0;
}