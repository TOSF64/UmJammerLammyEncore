#pragma once

#include "disc_reader.hpp"

#include <cstdint>
#include <string>

namespace encore {

    struct DirectoryRecord {
        std::uint32_t extent_location{};
        std::uint32_t data_length{};
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

} // namespace encore
