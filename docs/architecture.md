# DeltaVault System Architecture

DeltaVault is a content-addressable versioned backup and deduplication system implemented in C++17. It splits input files into variable-sized chunks using a rolling hash, hashes each chunk with SHA-256 for deduplication, and stores version history in sequential manifests.

## Module Overview & Team Ownership

### 1. Chunking & Hashing (`chunker.hpp/cpp`, `hasher.hpp/cpp` — Member A)
* **Chunker:** Implements content-defined chunking (CDC) using the Gear rolling fingerprint algorithm to split byte streams into variable-length chunks resistant to byte-shifting insertions.
* **Hasher:** Computes 256-bit cryptographic SHA-256 digests (`std::string` hex identifiers) for raw chunk payloads to uniquely address content.

### 2. Persistent Storage & Manifests (`object_store.hpp/cpp`, `manifest.hpp/cpp` — Member B)
* **Object Store:** Manages `.deltavault/objects/`, providing $O(1)$ content-addressed storage, retrieval, existence checks, enumeration, and deletion of raw binary chunk files indexed by their SHA-256 hash.
* **Manifest:** Manages `.deltavault/manifests/`, serializing and deserializing file version metadata (logical filename, version ID, timestamp, and the ordered sequence of chunk hashes required to reconstruct the file).

### 3. CLI, Vault Orchestration & Maintenance (`vault.hpp/cpp`, `main.cpp` — Member C)
* **CLI (`main.cpp`):** Parses user commands (`init`, `add`, `list`, `versions`, `restore`, `verify`, `stats`, `gc`), validates argument counts, and dispatches calls to the `Vault` controller.
* **Vault (`vault.hpp/cpp`):** Orchestrates end-to-end data flow between the Chunker/Hasher and ObjectStore/Manifest modules and executes system-wide algorithms:
  * **Integrity Verification (`verify`):** Linearly traverses a manifest's chunk list and recomputes SHA-256 digests against stored objects.
  * **Storage Statistics (`stats`):** Uses set union and reference counting across manifests to compute logical size, physical size, and deduplication savings.
  * **Garbage Collection (`gc`):** Uses a Mark-and-Sweep graph reachability algorithm to identify and delete unreferenced orphan chunk objects.