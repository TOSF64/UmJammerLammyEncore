#include "psx_exe.hpp"

#include <array>
#include <cstddef>
#include <stdexcept>

namespace encore {
    namespace {

        std::uint32_t read_le32(
            std::span<const std::uint8_t> data,
            std::size_t offset)
        {
            if (offset + 4 > data.size()) {
                throw std::runtime_error(
                    "PS-X EXE header field extends beyond available data.");
            }

            return
            static_cast<std::uint32_t>(data[offset]) |
            (static_cast<std::uint32_t>(data[offset + 1]) << 8) |
            (static_cast<std::uint32_t>(data[offset + 2]) << 16) |
            (static_cast<std::uint32_t>(data[offset + 3]) << 24);
        }

    } // namespace

    PsxExeHeader parse_psx_exe_header(
        std::span<const std::uint8_t> data)
    {
        constexpr std::size_t header_size = 0x800;

        if (data.size() < header_size) {
            throw std::runtime_error(
                "File is too small to contain a PS-X EXE header.");
        }

        constexpr std::array<std::uint8_t, 8> expected_magic{
            'P', 'S', '-', 'X', ' ', 'E', 'X', 'E'
        };

        for (std::size_t i = 0; i < expected_magic.size(); ++i) {
            if (data[i] != expected_magic[i]) {
                throw std::runtime_error(
                    "Boot executable does not contain PS-X EXE magic.");
            }
        }

        PsxExeHeader header;

        header.initial_pc    = read_le32(data, 0x10);
        header.initial_gp    = read_le32(data, 0x14);
        header.load_address  = read_le32(data, 0x18);
        header.file_size     = read_le32(data, 0x1C);
        header.bss_address   = read_le32(data, 0x28);
        header.bss_size      = read_le32(data, 0x2C);
        header.stack_address = read_le32(data, 0x30);
        header.stack_size    = read_le32(data, 0x34);

        return header;
    }

} // namespace encore
