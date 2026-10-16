#pragma once

#include <string>
#include <string_view>

namespace encore {

    struct SystemConfig {
        std::string boot_path;
    };

    [[nodiscard]]
    SystemConfig parse_system_cnf(std::string_view contents);

    [[nodiscard]]
    std::string normalize_playstation_disc_id(
        std::string_view boot_path);

} // namespace encore
