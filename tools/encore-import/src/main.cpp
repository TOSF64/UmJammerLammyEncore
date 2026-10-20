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
#include "system_cnf.hpp"
#include "psx_exe.hpp"
#include <algorithm>

namespace fs = std::filesystem;

namespace {

    constexpr std::string_view kSupportedBinSha256 =
    "26133b3186e3ff7ab3c6c8b34e1ddf2d5561fdffdb5457097d67d0924b59f56c";

    struct DiscInfo {
        std::string_view title;
        std::string_view region;
        std::string_view disc_id;
    };

    void extract_file(
        encore::DiscReader& disc,
        const encore::DirectoryEntry& entry,
        const fs::path& destination)
    {
        const auto data =
        disc.read_data(
            entry.extent_location,
            entry.data_length);

        fs::create_directories(destination.parent_path());

        std::ofstream output(
            destination,
            std::ios::binary);

        if (!output) {
            throw std::runtime_error(
                "Could not create output file: " + destination.string());
        }

        output.write(
            reinterpret_cast<const char*>(data.data()),
                     static_cast<std::streamsize>(data.size()));

        if (!output) {
            throw std::runtime_error(
                "Could not write output file: " + destination.string());
        }
    }

    std::string strip_iso_version(std::string_view identifier)
    {
        const auto semicolon = identifier.find(';');

        if (semicolon == std::string_view::npos) {
            return std::string(identifier);
        }

        return std::string(identifier.substr(0, semicolon));
    }

    void extract_directory_tree(
        encore::DiscReader& disc,
        const encore::DirectoryRecord& directory,
        std::uint16_t logical_block_size,
        const fs::path& destination)
    {
        fs::create_directories(destination);

        const auto entries =
        encore::read_directory(
            disc,
            directory,
            logical_block_size);

        for (const auto& entry : entries) {
            if (entry.identifier == "." || entry.identifier == "..") {
                continue;
            }

            const std::string output_name =
            strip_iso_version(entry.identifier);

            const fs::path output_path =
            destination / output_name;

            if (entry.is_directory()) {
                const encore::DirectoryRecord child_directory{
                    .extent_location = entry.extent_location,
                    .data_length = entry.data_length,
                };

                extract_directory_tree(
                    disc,
                    child_directory,
                    logical_block_size,
                    output_path);
            } else {
                extract_file(
                    disc,
                    entry,
                    output_path);
            }
        }
    }

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

void print_directory_tree(
    encore::DiscReader& disc,
    const encore::DirectoryRecord& directory,
    std::uint16_t logical_block_size,
    std::size_t depth = 0)
{
    const auto entries =
    encore::read_directory(
        disc,
        directory,
        logical_block_size);

    for (const auto& entry : entries) {
        // "." and ".." are useful directory records, but following them
        // recursively would send us around the filesystem forever.
        if (entry.identifier == "." || entry.identifier == "..") {
            continue;
        }

        const std::string indent(depth * 2, ' ');

        std::cout
        << indent
        << (entry.is_directory() ? "[DIR]  " : "[FILE] ")
        << entry.identifier << '\n'
        << indent
        << "  LBA:  " << entry.extent_location << '\n'
        << indent
        << "  Size: " << entry.data_length << " bytes\n";

        if (entry.is_directory()) {
            const encore::DirectoryRecord child_directory{
                .extent_location = entry.extent_location,
                .data_length = entry.data_length,
            };

            print_directory_tree(
                disc,
                child_directory,
                logical_block_size,
                depth + 1);
        }
    }
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

            if (digest == kSupportedBinSha256) {
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

                std::cout << "\nComplete filesystem tree:\n";

                print_directory_tree(
                    disc,
                    pvd.root_directory,
                    pvd.logical_block_size);
            }

            const auto system_cnf_it =
            std::find_if(
                root_entries.begin(),
                         root_entries.end(),
                         [](const encore::DirectoryEntry& entry) {
                             return entry.identifier == "SYSTEM.CNF;1";
                         });

            if (system_cnf_it == root_entries.end()) {
                throw std::runtime_error(
                    "SYSTEM.CNF;1 was not found in the root directory.");
            }

            const auto system_cnf_data =
            disc.read_data(
                system_cnf_it->extent_location,
                system_cnf_it->data_length);

            const std::string system_cnf_text(
                system_cnf_data.begin(),
                                              system_cnf_data.end());

            const auto system_config =
            encore::parse_system_cnf(system_cnf_text);

            const auto boot_disc_id =
            encore::normalize_playstation_disc_id(
                system_config.boot_path);

            const auto boot_separator =
            system_config.boot_path.find_last_of("\\/");

            const std::string boot_filename(
                system_config.boot_path.substr(
                    boot_separator == std::string::npos
                    ? 0
                    : boot_separator + 1));

            const auto boot_entry_it =
            std::find_if(
                root_entries.begin(),
                         root_entries.end(),
                         [&boot_filename](const encore::DirectoryEntry& entry) {
                             return entry.identifier == boot_filename;
                         });

            if (boot_entry_it == root_entries.end()) {
                throw std::runtime_error(
                    "Boot executable was not found in the root directory.");
            }

            const auto boot_executable_data =
            disc.read_data(
                boot_entry_it->extent_location,
                boot_entry_it->data_length);

            const auto exe_header =
            encore::parse_psx_exe_header(
                boot_executable_data);

            std::cout
            << "\nPlayStation boot configuration:\n"
            << "  BOOT:    " << system_config.boot_path << "\n"
            << "  Disc ID: " << boot_disc_id << "\n";

            std::cout
            << "\nPS-X EXE:\n"
            << std::hex
            << "  Initial PC:    0x" << exe_header.initial_pc << "\n"
            << "  Initial GP:    0x" << exe_header.initial_gp << "\n"
            << "  Load address:  0x" << exe_header.load_address << "\n"
            << std::dec
            << "  File size:     " << exe_header.file_size << " bytes\n"
            << std::hex
            << "  BSS address:   0x" << exe_header.bss_address << "\n"
            << std::dec
            << "  BSS size:      " << exe_header.bss_size << " bytes\n"
            << std::hex
            << "  Stack address: 0x" << exe_header.stack_address << "\n"
            << std::dec
            << "  Stack size:    " << exe_header.stack_size << " bytes\n";

        if (digest == kSupportedBinSha256) {
            std::cout
                << "Disc recognized!\n\n"
                << "  Title:   " << kPalEurope.title << '\n'
                << "  Region:  " << kPalEurope.region << '\n'
                << "  Disc ID: " << kPalEurope.disc_id << '\n'
                << "  Status:  Supported\n"
                << "  Integrity: Verified\n\n"
                << "I gotta believe!\n";

            std::cout
                << "\nEnter import destination: ";

            std::string output_input;
            std::getline(std::cin, output_input);

            if (output_input.empty()) {
                throw std::runtime_error(
                    "Import destination cannot be empty.");
                }

                const fs::path output_root =
                fs::path(output_input) / std::string(kPalEurope.disc_id);

                std::cout
                << "\nImport destination:\n"
                << "  " << output_root << '\n';

                std::cout
                << "\nExtracting filesystem...\n";

                extract_directory_tree(
                    disc,
                    pvd.root_directory,
                    pvd.logical_block_size,
                    output_root);

                std::cout
                << "\nFilesystem extraction complete:\n"
                << "  " << output_root << '\n';

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
