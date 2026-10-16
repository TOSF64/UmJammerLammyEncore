#pragma once

#include <cstdint>
#include <span>

namespace encore {

    struct PsxExeHeader {
        std::uint32_t initial_pc{};
        std::uint32_t initial_gp{};
        std::uint32_t load_address{};
        std::uint32_t file_size{};
        std::uint32_t bss_address{};
        std::uint32_t bss_size{};
        std::uint32_t stack_address{};
        std::uint32_t stack_size{};
    };

    [[nodiscard]]
    PsxExeHeader parse_psx_exe_header(
        std::span<const std::uint8_t> data);

} // namespace encore
