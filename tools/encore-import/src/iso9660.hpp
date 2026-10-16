#pragma once

#include "disc_reader.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace encore {

    struct DirectoryRecord {
        std::uint32_t extent_location{};
        std::uint32_t data_length{};
    };

    struct DirectoryEntry {
        std::string identifier;
        std::uint32_t extent_location{};
        std::uint32_t data_length{};
        std::uint8_t flags{};

        [[nodiscard]]
        bool is_directory() const
        {
            return (flags & 0x02u) != 0;
        }
    };

    struct PrimaryVolumeDescriptor {
        std::string system_identifier;
        std::string volume_identifier;
        std::uint32_t volume_space_size{};
        std::uint16_t logical_block_size{};
        DirectoryRecord root_directory;
    };

    [[nodiscard]]
    PrimaryVolumeDescriptor
    read_primary_volume_descriptor(DiscReader& disc);

    [[nodiscard]]
    std::vector<DirectoryEntry>
    read_directory(
        DiscReader& disc,
        const DirectoryRecord& directory,
        std::uint16_t logical_block_size);

} // namespace encore
