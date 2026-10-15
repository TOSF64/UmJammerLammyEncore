#include "disc_reader.hpp"

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

} // namespace encore
