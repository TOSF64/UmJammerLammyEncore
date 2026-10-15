#pragma once

#include "disc_reader.hpp"

#include <cstdint>
#include <string>

namespace encore {

struct PrimaryVolumeDescriptor {
    std::string system_identifier;
    std::string volume_identifier;
    std::uint32_t volume_space_size{};
    std::uint16_t logical_block_size{};
};

[[nodiscard]]
PrimaryVolumeDescriptor
read_primary_volume_descriptor(DiscReader& disc);

} // namespace encore
