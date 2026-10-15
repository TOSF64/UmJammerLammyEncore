#include "iso9660.hpp"

#include <algorithm>
#include <array>
#include <stdexcept>
#include <string_view>

namespace encore {
namespace {

std::string read_padded_string(
    const DiscReader::SectorData& sector,
    std::size_t offset,
    std::size_t length)
{
    std::string value(
        reinterpret_cast<const char*>(sector.data() + offset),
        length
    );

    while (!value.empty() &&
           (value.back() == ' ' || value.back() == '\0')) {
        value.pop_back();
    }

    return value;
}

std::uint16_t read_le16(
    const DiscReader::SectorData& sector,
    std::size_t offset)
{
    return
        static_cast<std::uint16_t>(sector[offset]) |
        (static_cast<std::uint16_t>(sector[offset + 1]) << 8);
}

std::uint32_t read_le32(
    const DiscReader::SectorData& sector,
    std::size_t offset)
{
    return
        static_cast<std::uint32_t>(sector[offset]) |
        (static_cast<std::uint32_t>(sector[offset + 1]) << 8) |
        (static_cast<std::uint32_t>(sector[offset + 2]) << 16) |
        (static_cast<std::uint32_t>(sector[offset + 3]) << 24);
}

std::uint32_t read_be32(
    const encore::DiscReader::SectorData& data,
    std::size_t offset)
{
    return
    (static_cast<std::uint32_t>(data[offset]) << 24) |
    (static_cast<std::uint32_t>(data[offset + 1]) << 16) |
    (static_cast<std::uint32_t>(data[offset + 2]) << 8) |
    static_cast<std::uint32_t>(data[offset + 3]);
}

std::uint32_t read_both_endian32(
    const encore::DiscReader::SectorData& data,
    std::size_t offset)
{
    const auto little = read_le32(data, offset);
    const auto big = read_be32(data, offset + 4);

    if (little != big) {
        throw std::runtime_error(
            "ISO 9660 both-endian 32-bit value does not match."
        );
    }

    return little;
}

} // namespace

PrimaryVolumeDescriptor
read_primary_volume_descriptor(DiscReader& disc)
{
    constexpr std::uint32_t pvd_logical_block = 16;

    const auto sector =
        disc.read_data_sector(pvd_logical_block);

    // ISO 9660 Primary Volume Descriptor:
    // byte 0     = descriptor type (1 = primary)
    // bytes 1-5  = "CD001"
    // byte 6     = descriptor version (1)

    if (sector[0] != 1) {
        throw std::runtime_error(
            "ISO 9660 Primary Volume Descriptor not found at logical block 16."
        );
    }

    constexpr std::string_view expected_identifier = "CD001";

    const std::string_view identifier(
        reinterpret_cast<const char*>(sector.data() + 1),
        5
    );

    if (identifier != expected_identifier) {
        throw std::runtime_error(
            "Invalid ISO 9660 standard identifier."
        );
    }

    if (sector[6] != 1) {
        throw std::runtime_error(
            "Unsupported ISO 9660 descriptor version."
        );
    }

    // We now know this is a valid Primary Volume Descriptor.

    constexpr std::size_t root_record_offset = 156;
    constexpr std::uint8_t minimum_root_record_length = 34;

    const auto root_record_length =
    sector[root_record_offset];

    if (root_record_length < minimum_root_record_length) {
        throw std::runtime_error(
            "ISO 9660 root directory record is too short."
        );
    }

    const auto root_extent_location =
    read_both_endian32(
        sector,
        root_record_offset + 2
    );

    const auto root_data_length =
    read_both_endian32(
        sector,
        root_record_offset + 10
    );

    const auto root_identifier_length =
    sector[root_record_offset + 32];

    if (root_identifier_length != 1 ||
        sector[root_record_offset + 33] != 0) {
        throw std::runtime_error(
            "Invalid ISO 9660 root directory identifier."
        );
        }

        constexpr std::uint8_t directory_flag = 0x02;

    const auto root_flags =
    sector[root_record_offset + 25];

    if ((root_flags & directory_flag) == 0) {
        throw std::runtime_error(
            "ISO 9660 root record is not marked as a directory."
        );
    }

    PrimaryVolumeDescriptor descriptor;

    descriptor.system_identifier =
        read_padded_string(sector, 8, 32);

    descriptor.volume_identifier =
        read_padded_string(sector, 40, 32);

    descriptor.volume_space_size =
        read_le32(sector, 80);

    descriptor.logical_block_size =
        read_le16(sector, 128);

    descriptor.root_directory.extent_location =
        root_extent_location;

    descriptor.root_directory.data_length =
        root_data_length;

    return descriptor;
}

} // namespace encore
