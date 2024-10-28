// Vita3K emulator project
// Copyright (C) 2024 Vita3K team
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

#include <packages/exfat.h>

namespace exfat {

static std::string get_exfat_file_name(fs::ifstream &img, uint8_t continuations) {
    std::vector<uint16_t> utf16Data;

    // Loop depending on the number of continuations, subtract 1 because continuations start to 2
    for (int cont = 0; cont < (continuations - 1); ++cont) {
        // read the name entry
        ExFATEntryName nameEntry;
        img.read(reinterpret_cast<char *>(&nameEntry), sizeof(ExFATEntryName));

        // Check if the entry is of type `0xC1`
        if (nameEntry.type == 0xC1) {
            // Adding the name characters to `utf16Data`
            for (int i = 0; i < EXFAT_ENAME_MAX; ++i)
                utf16Data.push_back(nameEntry.name[i]);
        } else {
            LOG_ERROR("Error: Unexpected type of continuation entry (expected 0xC1, found: 0x{:X}, on offset: {})", nameEntry.type, static_cast<int>(img.tellg()));
            break;
        }
    }

    // Adding null-terminator
    utf16Data.push_back(0);

    // Convert name to UTF-8
    std::string utf8String;
    for (uint16_t ch : utf16Data) {
        if (ch == 0) {
            break;
        }
        if (ch < 0x80) {
            utf8String.push_back(static_cast<char>(ch));
        } else if (ch < 0x800) {
            utf8String.push_back(0xC0 | (ch >> 6));
            utf8String.push_back(0x80 | (ch & 0x3F));
        } else {
            utf8String.push_back(0xE0 | (ch >> 12));
            utf8String.push_back(0x80 | ((ch >> 6) & 0x3F));
            utf8String.push_back(0x80 | (ch & 0x3F));
        }
    }

    return utf8String;
}

static uint64_t get_cluster_offset(const ExFATSuperBlock &super_block, uint32_t cluster) {
    const uint64_t sector_size = static_cast<uint64_t>(1 << super_block.sector_bits);
    const uint64_t sectors_per_cluster = static_cast<uint64_t>(1 << super_block.spc_bits);
    const uint64_t cluster_size = sector_size * sectors_per_cluster;

    // The cluster number is 0-based, so we need to add 1
    return static_cast<uint64_t>(cluster + 1) * cluster_size;
}

static void traverse_directory(fs::ifstream &img, const uint64_t img_size, std::vector<std::streampos> &offset_stack, const ExFATSuperBlock &super_block,
    const uint32_t cluster, const fs::path &output_path, fs::path current_dir) {
    // Seek to the cluster offset
    img.seekg(get_cluster_offset(super_block, cluster));

    // Loop through the entries in the clustor
    while (img.tellg() < img_size) {
        // Read the file entry
        ExFATFileEntry file_entry;
        img.read(reinterpret_cast<char *>(&file_entry), sizeof(ExFATFileEntry));

        switch (file_entry.type) {
        case EXFAT_ENTRY_BITMAP:
        case EXFAT_ENTRY_UPCASE:
        case EXFAT_ENTRY_LABEL:
            // Skip these entries
            break;
        case EXFAT_ENTRY_FILE: {
            // Read the file info
            ExFATFileEntryInfo file_info;
            img.read(reinterpret_cast<char *>(&file_info), sizeof(ExFATFileEntryInfo));

            // Get the name of file or directory
            std::string name = get_exfat_file_name(img, file_entry.continuations);

            // Set path of the current file or directory
            const auto subdir = current_dir / name;
            const auto current_output_path = output_path / subdir;

            // Get the current offset
            const uint64_t current_offset = img.tellg();

            if (file_entry.attrib & EXFAT_ATTRIB_DIR) {
                fs::create_directories(current_output_path);

                // Store the current offset in the stack
                offset_stack.push_back(current_offset);

                // Traverse the directory recursively
                traverse_directory(img, img_size, offset_stack, super_block, file_info.start_cluster, output_path, subdir);
            } else if (file_entry.attrib & EXFAT_ATTRIB_ARCH) {
                // Seek to the file offset
                img.seekg(get_cluster_offset(super_block, file_info.start_cluster));

                // Create the file and write the data
                fs::ofstream output_file(current_output_path, std::ios::binary);
                const auto file_size = file_info.size;
                std::vector<char> buffer(file_size);
                img.read(buffer.data(), file_size);
                output_file.write(buffer.data(), file_size);
                output_file.close();

                // Go back to the current offset for the next entry
                img.seekg(current_offset);
            }
            break;
        }
        default:
            // End of directory: return to parent directory and previous offset if available, otherwise seek to the end of the image
            if (!offset_stack.empty()) {
                img.seekg(offset_stack.back());
                offset_stack.pop_back();
                current_dir = current_dir.parent_path();
            } else {
                img.seekg(0, std::ios::end);
            }
            break;
        }
    }
}

void extract_exfat(const fs::path &partition_path, const std::string &partition, const fs::path &pref_path) {
    // Open the partition file for reading in binary mode
    fs::ifstream img(partition_path / partition, std::ios::binary);
    if (!img.is_open()) {
        LOG_ERROR("Failed to open partition file");
        return;
    }

    // Get the size of the partition file
    const uint64_t img_size = fs::file_size(partition_path / partition);

    // Stack to keep track of the offsets
    std::vector<std::streampos> offset_stack;

    // Read the super block
    ExFATSuperBlock super_block;
    img.read(reinterpret_cast<char *>(&super_block), sizeof(ExFATSuperBlock));

    // Set output path
    const fs::path output_path{ pref_path / partition.substr(0, 3) };

    // Current directory
    fs::path current_dir;

    // Traverse the root directory
    traverse_directory(img, img_size, offset_stack, super_block, super_block.rootdir_cluster, output_path, current_dir);

    // Close the partition file
    img.close();
}

} // namespace exfat
