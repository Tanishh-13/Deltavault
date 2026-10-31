
# DeltaVault Storage Format

## Directory Layout

A DeltaVault vault stores unique chunk objects and file manifests.

```text
vault/
├── objects/
│   ├── chunk_id_1
│   └── chunk_id_2
└── manifests/
    ├── notes.txt.v1.manifest
    └── notes.txt.v2.manifest
```

## Objects

Each file in `objects/` contains the raw binary bytes of one chunk. The filename is the chunk's object ID.

Object IDs may contain ASCII letters, digits, hyphens, and underscores. An object ID must not be empty.

When an object with an existing ID is stored again, DeltaVault reuses the existing object without overwriting its bytes. This allows multiple files or versions to reference the same stored chunk.

## Manifests

Each manifest represents one version of a logical file. Its filename follows this pattern:

`<logical_filename>.v<version>.manifest`

The manifest is a UTF-8-compatible text file with the following line-based structure:

1. Logical filename
2. Version number
3. Number of chunks
4. One chunk ID per line, in original file order

For example:

```text
notes.txt
1
3
chunk_A
chunk_B
chunk_C
```

This manifest represents version 1 of `notes.txt`, consisting of three chunks in the order `chunk_A`, `chunk_B`, `chunk_C`.

## Versioning

Each version has its own manifest. Saving a new version does not require duplicating chunks that already exist in the object store.

## Restoration

To restore a file, DeltaVault reads the manifest, loads each referenced object in order, and concatenates the chunk bytes. Preserving chunk order is necessary to reconstruct the original file correctly.

## Current Limitations

- Logical filenames are restricted to ASCII letters, digits, dots, hyphens, and underscores.
- Version numbers must be greater than zero.
- Object IDs and chunk IDs must be non-empty and use ASCII letters, digits, hyphens, and underscores.
- The format does not currently include checksums, a format version, or recovery metadata.
