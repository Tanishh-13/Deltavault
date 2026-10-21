#include "deltavault/vault.hpp"
#include <iostream>
#include <string>

void print_usage() {
    std::cout << "DeltaVault CLI Usage:\n"
              << "  deltavault init [<vault_path>]\n"
              << "  deltavault add <file>\n"
              << "  deltavault list\n"
              << "  deltavault versions <file>\n"
              << "  deltavault restore <file> <version> <output>\n"
              << "  deltavault verify <file> <version>\n"
              << "  deltavault stats\n"
              << "  deltavault gc\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Error: Missing command.\n\n";
        print_usage();
        return 1;
    }

    std::string command = argv[1];

    if (command == "init") {
        std::filesystem::path vault_path = (argc >= 3) ? argv[2] : ".deltavault";
        deltavault::Vault vault(vault_path);
        if (vault.init()) {
            std::cout << "Initialized empty DeltaVault at " << std::filesystem::absolute(vault_path) << "\n";
            return 0;
        } else {
            std::cerr << "Error: Initialization failed.\n";
            return 1;
        }
    }

    // Vault instance in current directory for standard commands
    deltavault::Vault vault(".deltavault");

    if (command == "add") {
        if (argc < 3) {
            std::cerr << "Error: 'add' requires a <file> argument.\n";
            return 1;
        }
        if (!vault.add(argv[2])) {
            return 1;
        }
        return 0;
    } else if (command == "list") {
        auto files = vault.list();
        if (files.empty()) {
            std::cout << "Vault has no tracked files.\n";
        } else {
            for (const auto& file : files) {
                std::cout << file << "\n";
            }
        }
        return 0;
    } else if (command == "versions") {
        if (argc < 3) {
            std::cerr << "Error: 'versions' requires a <file> argument.\n";
            return 1;
        }
        std::cout << "Command 'versions' parsed successfully (pending Day 3).\n";
        return 0;
    } else if (command == "restore") {
        if (argc < 5) {
            std::cerr << "Error: 'restore' requires <file>, <version>, and <output> arguments.\n";
            return 1;
        }
        std::cout << "Command 'restore' parsed successfully (pending Day 3).\n";
        return 0;
    } else if (command == "verify") {
        if (argc < 4) {
            std::cerr << "Error: 'verify' requires <file> and <version> arguments.\n";
            return 1;
        }
        std::cout << "Command 'verify' parsed successfully (pending Day 4).\n";
        return 0;
    } else if (command == "stats") {
        std::cout << "Command 'stats' parsed successfully (pending Day 4).\n";
        return 0;
    } else if (command == "gc") {
        std::cout << "Command 'gc' parsed successfully (pending Day 4).\n";
        return 0;
    } else {
        std::cerr << "Error: Unknown command '" << command << "'.\n\n";
        print_usage();
        return 1;
    }
}