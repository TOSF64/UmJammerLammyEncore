#include "disc_reader.hpp"

#include <algorithm>
#include <stdexcept>

namespace encore {

DiscReader::DiscReader(const std::filesystem::path& bin_path)
    : file_(bin_path, std::ios::binary)
{
    if (!file_) {
        throw std::runtime_error(
            "Could not open disc image: " + bin_path.string()
        );
    }
}

DiscReader::SectorData
DiscReader::read_data_sector(std::uint32_t logical_block)
{
    const auto offset =
        static_cast<std::streamoff>(logical_block) *
            static_cast<std::streamoff>(raw_sector_size) +
        static_cast<std::streamoff>(mode2_user_data_offset);

    file_.clear();
    file_.seekg(offset, std::ios::beg);

    if (!file_) {
        throw std::runtime_error(
            "Could not seek to disc logical block."
        );
    }

    SectorData data{};

    file_.read(
        reinterpret_cast<char*>(data.data()),
        static_cast<std::streamsize>(data.size())
    );

    if (file_.gcount() !=
        static_cast<std::streamsize>(data.size())) {
        throw std::runtime_error(
            "Could not read complete disc logical block."
        );
    }

    return data;
}

std::vector<std::uint8_t>
DiscReader::read_data(
    std::uint32_t logical_block,
    std::uint32_t byte_count)
{
    std::vector<std::uint8_t> result;
    result.reserve(byte_count);

    std::uint32_t current_block = logical_block;
    std::uint32_t remaining = byte_count;

    while (remaining > 0) {
        const auto sector = read_data_sector(current_block);

        const auto bytes_to_copy =
        std::min<std::uint32_t>(
            remaining,
            static_cast<std::uint32_t>(sector.size()));

        result.insert(
            result.end(),
                      sector.begin(),
                      sector.begin() + bytes_to_copy);

        remaining -= bytes_to_copy;
        ++current_block;
    }

    return result;
}

} // namespace encore
