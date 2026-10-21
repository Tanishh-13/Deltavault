#include "deltavault/vault.hpp"
#include <cassert>
#include <iostream>
#include <filesystem>
#include <fstream>

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

void test_reinit_existing_vault() {
    std::filesystem::path test_dir = "./tmp_test_vault_exists";
    if (std::filesystem::exists(test_dir)) {
        std::filesystem::remove_all(test_dir);
    }

    deltavault::Vault vault(test_dir);
    assert(vault.init());

    // Re-initialization on valid existing vault must fail safely
    assert(!vault.init());

    std::filesystem::remove_all(test_dir);
    std::cout << "[PASS] test_reinit_existing_vault\n";
}

void test_invalid_vault_path() {
    // 1. Empty path must fail
    deltavault::Vault empty_vault("");
    assert(!empty_vault.is_valid());
    assert(!empty_vault.init());

    // 2. Path pointing to an existing regular file (not a directory) must fail
    std::filesystem::path file_path = "./tmp_dummy_file.txt";
    {
        std::ofstream ofs(file_path);
        ofs << "not a directory";
    }
    deltavault::Vault file_vault(file_path);
    assert(!file_vault.is_valid());
    assert(!file_vault.init());

    std::filesystem::remove(file_path);
    std::cout << "[PASS] test_invalid_vault_path\n";
}

int main() {
    std::cout << "Running Member C Day 1 Vault Tests...\n";
    test_vault_init();
    test_reinit_existing_vault();
    test_invalid_vault_path();
    std::cout << "All Day 1 Vault Tests Passed Successfully!\n";
    return 0;
}