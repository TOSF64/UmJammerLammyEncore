#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>

namespace encore {

class DiscReader {
public:
    static constexpr std::size_t raw_sector_size = 2352;
    static constexpr std::size_t user_data_size = 2048;
    static constexpr std::size_t mode2_user_data_offset = 24;

    using SectorData = std::array<std::uint8_t, user_data_size>;

    explicit DiscReader(const std::filesystem::path& bin_path);

    [[nodiscard]]
    SectorData read_data_sector(std::uint32_t logical_block);

private:
    std::ifstream file_;
};

} // namespace encore
