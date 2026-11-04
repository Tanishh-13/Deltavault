#include "deltavault/vault.hpp"
#include "deltavault/manifest.hpp"
#include <cassert>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <vector>

void write_bytes(const std::filesystem::path& path, const std::vector<std::uint8_t>& data) {
    std::ofstream ofs(path, std::ios::binary | std::ios::trunc);
    if (!data.empty()) {
        ofs.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
    }
}

void write_text(const std::filesystem::path& path, const std::string& text) {
    std::ofstream ofs(path, std::ios::binary | std::ios::trunc);
    ofs << text;
}

std::vector<std::uint8_t> read_bytes(const std::filesystem::path& path) {
    std::ifstream ifs(path, std::ios::binary);
    return std::vector<std::uint8_t>((std::istreambuf_iterator<char>(ifs)),
                                     std::istreambuf_iterator<char>());
}

void test_acceptance_1_init_and_empty_file() {
    std::filesystem::path test_dir = "./tmp_acc_1";
    std::filesystem::remove_all(test_dir);

    deltavault::Vault vault(test_dir);
    assert(!vault.is_valid());
    assert(vault.init());
    assert(vault.is_valid());
    // Re-init must fail safely
    assert(!vault.init());

    // Test 0-byte empty file
    std::filesystem::path empty_file = "./empty.txt";
    write_text(empty_file, "");
    assert(vault.add(empty_file));

    std::filesystem::path restored_empty = "./empty_restored.txt";
    assert(vault.verify("empty.txt", "v1"));
    assert(vault.restore("empty.txt", "v1", restored_empty));
    assert(read_bytes(restored_empty).empty());

    std::filesystem::remove(empty_file);
    std::filesystem::remove(restored_empty);
    std::filesystem::remove_all(test_dir);
    std::cout << "[PASS] Acceptance 1: Init & Empty File Handling\n";
}

void test_acceptance_2_large_multichunk_dedup_and_restore() {
    std::filesystem::path test_dir = "./tmp_acc_2";
    std::filesystem::remove_all(test_dir);

    deltavault::Vault vault(test_dir);
    assert(vault.init());

    // Create 300 KB deterministic binary payload (spans multiple chunks)
    std::vector<std::uint8_t> v1_data(300 * 1024);
    for (size_t i = 0; i < v1_data.size(); ++i) {
        v1_data[i] = static_cast<std::uint8_t>((i * 31 + 17) % 256);
    }

    std::filesystem::path bin_file = "./dataset.bin";
    write_bytes(bin_file, v1_data);
    assert(vault.add(bin_file)); // v1

    // Modify only the last 100 bytes for v2 (earlier chunks should deduplicate)
    std::vector<std::uint8_t> v2_data = v1_data;
    for (size_t i = v2_data.size() - 100; i < v2_data.size(); ++i) {
        v2_data[i] = 0xFF;
    }
    write_bytes(bin_file, v2_data);
    assert(vault.add(bin_file)); // v2

    // Simulate restarting program by instantiating a fresh Vault object pointing to same path
    deltavault::Vault reopened_vault(test_dir);
    auto vers = reopened_vault.versions("dataset.bin");
    assert(vers.size() == 2 && vers[0] == "v1" && vers[1] == "v2");

    // Verify deduplication occurred: physical bytes must be significantly less than logical bytes (600 KB)
    auto st = reopened_vault.stats();
    assert(st.logical_bytes == v1_data.size() + v2_data.size());
    assert(st.physical_bytes < st.logical_bytes);
    assert(st.deduplication_savings > 20.0);

    // Byte-for-byte restore of both v1 and v2
    std::filesystem::path out_v1 = "./dataset_v1.bin";
    std::filesystem::path out_v2 = "./dataset_v2.bin";
    assert(reopened_vault.restore("dataset.bin", "v1", out_v1));
    assert(reopened_vault.restore("dataset.bin", "v2", out_v2));
    assert(read_bytes(out_v1) == v1_data);
    assert(read_bytes(out_v2) == v2_data);

    std::filesystem::remove(bin_file);
    std::filesystem::remove(out_v1);
    std::filesystem::remove(out_v2);
    std::filesystem::remove_all(test_dir);
    std::cout << "[PASS] Acceptance 2: Multi-Chunk Deduplication, Reopen & Byte-for-Byte Restore\n";
}

void test_acceptance_3_corruption_and_gc() {
    std::filesystem::path test_dir = "./tmp_acc_3";
    std::filesystem::remove_all(test_dir);

    deltavault::Vault vault(test_dir);
    assert(vault.init());

    std::filesystem::path sample = "./report.txt";
    write_text(sample, "Confidential Financial Report v1");
    assert(vault.add(sample)); // v1
    assert(vault.add(sample)); // v2 (shares chunk with v1)

    // Plant unreferenced orphan chunk
    std::filesystem::path orphan = vault.get_objects_dir() / "deadbeef00000000000000000000000000000000000000000000000000000000";
    write_text(orphan, "orphan_data");
    assert(vault.stats().chunk_count == 2);

    // GC must remove orphan and preserve shared chunk
    assert(vault.gc());
    assert(!std::filesystem::exists(orphan));
    assert(vault.stats().chunk_count == 1);
    assert(vault.verify("report.txt", "v1"));
    assert(vault.verify("report.txt", "v2"));

    // Corrupt the shared chunk on disk
    deltavault::ManifestStore mstore(test_dir);
    auto m = mstore.load("report.txt", 1);
    write_text(vault.get_objects_dir() / m.chunk_ids[0], "CORRUPTED_BYTES");

    // Both verify and restore must detect corruption and fail safely
    assert(!vault.verify("report.txt", "v1"));
    assert(!vault.restore("report.txt", "v1", "./corrupted_out.txt"));

    std::filesystem::remove(sample);
    std::filesystem::remove("./corrupted_out.txt");
    std::filesystem::remove_all(test_dir);
    std::cout << "[PASS] Acceptance 3: Tamper Detection & Safe Garbage Collection\n";
}

int main() {
    std::cout << "=== Running DeltaVault Day 5 Full Acceptance Test Suite ===\n";
    test_acceptance_1_init_and_empty_file();
    test_acceptance_2_large_multichunk_dedup_and_restore();
    test_acceptance_3_corruption_and_gc();
    std::cout << "=== All Day 5 Acceptance Tests Passed Successfully! ===\n";
    return 0;
}