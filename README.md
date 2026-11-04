# DeltaVault

DeltaVault is a content-addressable, deduplicating versioned file backup utility written in C++17. It splits files into variable-size chunks using Gear-based Content-Defined Chunking (CDC), identifies chunks by their cryptographic SHA-256 digest, stores unique chunks only once on disk, and tracks file history through versioned manifests.

---

## Features

- **Content-Defined Chunking (CDC):** Rolling Gear hash splits files into chunks between `8 KiB` and `128 KiB` (targeting `64 KiB`), ensuring local file edits only produce new chunks for modified regions.
- **SHA-256 Content Addressing:** Every chunk is named and verified by its 64-character hex SHA-256 hash.
- **Cross-Version & Cross-File Deduplication:** Identical chunks across different versions or files are stored once in `.deltavault/objects/`.
- **Cryptographic Integrity Verification:** `verify` and `restore` recompute SHA-256 hashes of every referenced chunk to detect bit-rot or tampering.
- **Mark-and-Sweep Garbage Collection:** Reclaims disk space from unreferenced chunk objects while preserving chunks shared by retained manifests.

---

## Build Instructions

### Prerequisites
- A C++17 compliant compiler (`g++` 9+ or MinGW-w64 UCRT `g++` on Windows).

### Compiling the CLI

**Linux / macOS:**
```bash
g++ -std=c++17 -O2 -Wall -Wextra -Iinclude src/main.cpp src/vault.cpp src/chunker.cpp src/object_store.cpp src/hasher.cpp src/manifest.cpp -o deltavault