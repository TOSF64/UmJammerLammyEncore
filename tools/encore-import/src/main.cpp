#include <openssl/evp.h>

#include "disc_reader.hpp"
#include "iso9660.hpp"
#include <array>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace fs = std::filesystem;

namespace {

constexpr std::string_view kSupportedBinSha256 =
    "26133b3186e3ff7ab3c6c8b34e1ddf2d5561fdffdb5457097d67d0924b59f56c";

struct DiscInfo {
    std::string_view title;
    std::string_view region;
    std::string_view disc_id;
};

constexpr DiscInfo kPalEurope{
    "Um Jammer Lammy",
    "Europe (PAL)",
    "SCES-01753"
};

std::string trim_left(std::string value)
{
    const auto pos = value.find_first_not_of(" \t\r\n");

    if (pos == std::string::npos) {
        return {};
    }

    value.erase(0, pos);
    return value;
}

std::optional<std::string> parse_bin_filename(const fs::path& cue_path)
{
    std::ifstream cue(cue_path);

    if (!cue) {
        throw std::runtime_error("Could not open CUE file.");
    }

    std::string line;

    while (std::getline(cue, line)) {
        line = trim_left(std::move(line));

        if (!line.starts_with("FILE")) {
            continue;
        }

        const auto first_quote = line.find('"');
        const auto second_quote =
            first_quote == std::string::npos
                ? std::string::npos
                : line.find('"', first_quote + 1);

        if (first_quote == std::string::npos ||
            second_quote == std::string::npos) {
            throw std::runtime_error(
                "Found a FILE entry in the CUE, but could not parse its filename."
            );
        }

        return line.substr(
            first_quote + 1,
            second_quote - first_quote - 1
        );
    }

    return std::nullopt;
}

std::string sha256_file(const fs::path& path)
{
    std::ifstream input(path, std::ios::binary);

    if (!input) {
        throw std::runtime_error(
            "Could not open BIN file: " + path.string()
        );
    }

    using ContextPtr =
        std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)>;

    ContextPtr context(EVP_MD_CTX_new(), EVP_MD_CTX_free);

    if (!context) {
        throw std::runtime_error("Could not create SHA-256 context.");
    }

    if (EVP_DigestInit_ex(context.get(), EVP_sha256(), nullptr) != 1) {
        throw std::runtime_error("Could not initialize SHA-256.");
    }

    std::array<char, 1024 * 1024> buffer{};

    while (input) {
        input.read(buffer.data(), buffer.size());
        const auto bytes_read = input.gcount();

        if (bytes_read > 0) {
            if (EVP_DigestUpdate(
                    context.get(),
                    buffer.data(),
                    static_cast<std::size_t>(bytes_read)
                ) != 1) {
                throw std::runtime_error("SHA-256 update failed.");
            }
        }
    }

    if (!input.eof()) {
        throw std::runtime_error("Error while reading BIN file.");
    }

    std::array<unsigned char, EVP_MAX_MD_SIZE> digest{};
    unsigned int digest_length = 0;

    if (EVP_DigestFinal_ex(
            context.get(),
            digest.data(),
            &digest_length
        ) != 1) {
        throw std::runtime_error("Could not finalize SHA-256.");
    }

    std::ostringstream result;
    result << std::hex << std::setfill('0');

    for (unsigned int i = 0; i < digest_length; ++i) {
        result << std::setw(2)
               << static_cast<unsigned int>(digest[i]);
    }

    return result.str();
}

void print_header()
{
    std::cout
        << "Um Jammer Lammy: Encore! Importer\n"
        << "---------------------------------\n\n";
}

} // namespace

int main(int argc, char* argv[])
{
    print_header();

    if (argc != 2) {
        std::cerr
            << "Usage:\n"
            << "  encore-import <path-to-cue>\n";

        return 2;
    }

    try {
        const fs::path cue_path =
            fs::absolute(fs::path(argv[1]));

        if (!fs::exists(cue_path)) {
            throw std::runtime_error(
                "CUE file does not exist: " + cue_path.string()
            );
        }

        std::cout
            << "Reading CUE sheet...\n"
            << "  " << cue_path << "\n\n";

        const auto bin_filename =
            parse_bin_filename(cue_path);

        if (!bin_filename) {
            throw std::runtime_error(
                "No FILE entry was found in the CUE sheet."
            );
        }

        const fs::path bin_path =
            cue_path.parent_path() / *bin_filename;

        std::cout
            << "Referenced BIN:\n"
            << "  " << bin_path.filename() << "\n\n";

        if (!fs::exists(bin_path)) {
            throw std::runtime_error(
                "Referenced BIN file does not exist: " +
                bin_path.string()
            );
        }

        std::cout
            << "Calculating SHA-256...\n";

        const std::string digest =
            sha256_file(bin_path);

            std::cout
            << "  " << digest << "\n\n";

            std::cout
            << "Inspecting ISO 9660 filesystem...\n";

            encore::DiscReader disc(bin_path);

            const auto pvd =
            encore::read_primary_volume_descriptor(disc);

            const auto root_entries =
            encore::read_directory(
                disc,
                pvd.root_directory,
                pvd.logical_block_size);

            std::cout
            << "  Standard:           ISO 9660\n"
            << "  System identifier:  "
            << (pvd.system_identifier.empty()
            ? "(empty)"
            : pvd.system_identifier)
            << '\n'
            << "  Volume identifier:  "
            << (pvd.volume_identifier.empty()
            ? "(empty)"
            : pvd.volume_identifier)
            << '\n'
            << "  Logical block size: "
            << pvd.logical_block_size
            << " bytes\n"
            << "  Volume blocks:      "
            << pvd.volume_space_size
            << '\n'
            << "  Root extent LBA:    "
            << pvd.root_directory.extent_location
            << '\n'
            << "  Root size:          "
            << pvd.root_directory.data_length
            << " bytes\n\n";

            std::cout << "\nRoot directory:\n";

            for (const auto& entry : root_entries) {
                std::cout
                << "  "
                << (entry.is_directory() ? "[DIR]  " : "[FILE] ")
                << entry.identifier
                << "\n"
                << "         LBA:  " << entry.extent_location << "\n"
                << "         Size: " << entry.data_length << " bytes\n";
            }

        if (digest == kSupportedBinSha256) {
            std::cout
                << "Disc recognized!\n\n"
                << "  Title:   " << kPalEurope.title << '\n'
                << "  Region:  " << kPalEurope.region << '\n'
                << "  Disc ID: " << kPalEurope.disc_id << '\n'
                << "  Status:  Supported\n"
                << "  Integrity: Verified\n\n"
                << "I gotta believe!\n";

            return 0;
        }

        std::cout
            << "Disc image is not currently recognized by Encore.\n"
            << "No files were modified.\n";

        return 1;
    }
    catch (const std::exception& error) {
        std::cerr
            << "Import error: "
            << error.what()
            << '\n';

        return 1;
    }
}
