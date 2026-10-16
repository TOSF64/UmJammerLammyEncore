#include "system_cnf.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>

namespace encore {
    namespace {

        std::string trim(std::string_view value)
        {
            const auto first =
            std::find_if_not(
                value.begin(),
                             value.end(),
                             [](unsigned char ch) {
                                 return std::isspace(ch);
                             });

            const auto last =
            std::find_if_not(
                value.rbegin(),
                             value.rend(),
                             [](unsigned char ch) {
                                 return std::isspace(ch);
                             }).base();

                             if (first >= last) {
                                 return {};
                             }

                             return std::string(first, last);
        }

    } // namespace

    SystemConfig parse_system_cnf(std::string_view contents)
    {
        SystemConfig config;

        std::size_t line_start = 0;

        while (line_start < contents.size()) {
            const auto line_end =
            contents.find_first_of("\r\n", line_start);

            const auto line =
            contents.substr(
                line_start,
                line_end == std::string_view::npos
                ? std::string_view::npos
                : line_end - line_start);

            const auto equals = line.find('=');

            if (equals != std::string_view::npos) {
                const auto key = trim(line.substr(0, equals));
                const auto value = trim(line.substr(equals + 1));

                if (key == "BOOT") {
                    config.boot_path = value;
                }
            }

            if (line_end == std::string_view::npos) {
                break;
            }

            line_start = line_end + 1;

            if (line_start < contents.size() &&
                contents[line_end] == '\r' &&
                contents[line_start] == '\n') {
                ++line_start;
                }
        }

        if (config.boot_path.empty()) {
            throw std::runtime_error(
                "SYSTEM.CNF does not contain a BOOT entry.");
        }

        return config;
    }

    std::string normalize_playstation_disc_id(
        std::string_view boot_path)
    {
        const auto slash = boot_path.find_last_of("\\/");

        auto filename =
        boot_path.substr(
            slash == std::string_view::npos ? 0 : slash + 1);

        const auto version = filename.find(';');

        if (version != std::string_view::npos) {
            filename = filename.substr(0, version);
        }

        std::string id(filename);

        std::replace(id.begin(), id.end(), '_', '-');
        id.erase(
            std::remove(id.begin(), id.end(), '.'),
                 id.end());

        return id;
    }

} // namespace encore
