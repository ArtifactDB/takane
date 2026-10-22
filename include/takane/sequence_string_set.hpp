#ifndef TAKANE_SEQUENCE_STRING_SET_HPP
#define TAKANE_SEQUENCE_STRING_SET_HPP

#include "byteme/byteme.hpp"
#include "ritsuko/ritsuko.hpp"
#include "sanisizer/sanisizer.hpp"

#include <array>
#include <algorithm>
#include <stdexcept>
#include <limits>
#include <cstddef>
#include <filesystem>
#include <exception>

#include "utils_other.hpp"

/**
 * @file sequence_string_set.hpp
 * @brief Validation for sequence string sets.
 */

namespace takane {

/**
 * @cond
 */
inline constexpr int char2int(char val) {
    return static_cast<int>(val) - static_cast<int>(std::numeric_limits<char>::min());
}

typedef std::array<bool, char2int(std::numeric_limits<char>::max())> AllowedCharacterMap;

template<bool has_quality_>
std::size_t parse_sequence_contents(const std::filesystem::path& path, AllowedCharacterMap allowed, char lowest_quality, bool parallel) {
    auto gzreader = open_reader<byteme::GzipFileReader>(path, byteme::GzipFileReaderOptions());
    auto pb = wrap_reader_for_bytes<char>(std::move(gzreader), parallel);

    std::size_t nseq = 0;
    unsigned long long line_count = 1;
    auto advance_and_check = [&]() -> char {
        if (!pb->advance()) {
            throw std::runtime_error("premature end of the file at line " + ritsuko::itoa(line_count));
        }
        return pb->get();
    };

    while (pb->valid()) {
        // Chomp through any empty lines.
        while (pb->get() == '\n') {
            ++line_count;
            if (!pb->advance()) {
                break;
            }
        }
        if (!pb->valid()) {
            break;
        }

        // Processing the name. 
        {
            if constexpr(!has_quality_) {
                if (pb->get() != '>') {
                    throw std::runtime_error("sequence name should start with '>' at line " + ritsuko::itoa(line_count));
                }
            } else {
                if (pb->get() != '@') {
                    throw std::runtime_error("sequence name should start with '@' at line " + ritsuko::itoa(line_count));
                }
            }

            const std::size_t mult_limit = nseq / 10;
            const std::size_t add_limit = nseq % 10;
            std::size_t proposed = 0;
            bool empty = true;
            while (true) {
                char val = advance_and_check();
                if (val == '\n') {
                    break;
                }

                empty = false;
                if (val < '0' || val > '9') {
                    throw std::runtime_error("sequence name should be a non-negative integer at line " + ritsuko::itoa(line_count));
                }
                
                const auto delta = val - '0';
                if (proposed < mult_limit || (proposed == mult_limit && sanisizer::is_less_than_or_equal(delta, add_limit))) {
                    proposed *= 10;
                    proposed += delta;
                } else {
                    throw std::runtime_error("sequence name is out of bounds at line " + ritsuko::itoa(line_count));
                }
            }
            if (empty || proposed != nseq) {
                throw std::runtime_error("sequence name should be its index at line " + ritsuko::itoa(line_count));
            }
            ++line_count;
        }

        if constexpr(!has_quality_) {
            // Processing the sequence itself until we get to a '>' or we run out of bytes.
            char val = advance_and_check();
            while (true) {
                if (val == '\n') {
                    ++line_count;
                    if (!pb->advance()) {
                        break;
                    }
                    val = pb->get();
                    if (val == '>') {
                        break;
                    }
                } else if (allowed[char2int(val)]) {
                    val = advance_and_check();
                } else {
                    throw std::runtime_error("forbidden character '" + std::string(1, val) + "' in sequence at line " + ritsuko::itoa(line_count));
                }
            }

        } else {
            // Processing the sequence itself until we get to a '+'.
            std::size_t seq_length = 0;
            {
                char val = advance_and_check();
                while (true) {
                    if (val == '\n') {
                        ++line_count;
                        val = advance_and_check();
                        if (val == '+') {
                            break;
                        }
                    } else if (allowed[char2int(val)]) {
                        seq_length = sanisizer::sum<std::size_t>(seq_length, 1); // Protect against overflow as this is important for the quality string checks.
                        val = advance_and_check();
                    } else {
                        throw std::runtime_error("forbidden character '" + std::string(1, val) + "' in sequence at line " + ritsuko::itoa(line_count));
                    }
                }
            }

            // Next should be a single line; starting with '+' is implicit from above.
            while (true) {
                const char val = advance_and_check();
                if (val == '\n') {
                    ++line_count;
                    break;
                }
            }

            // Processing the qualities. Note that we can't check for '@' as a
            // delimitor, as this can be a valid score, so instead we check at each
            // newline whether we've reached the specified length, and quit if so.
            std::size_t qual_length = 0;
            while (true) {
                const char val = advance_and_check();
                if (val == '\n') {
                    ++line_count;
                    if (qual_length >= seq_length) {
                        break;
                    }
                } else if (val >= lowest_quality && val <= '~') {
                    qual_length = sanisizer::sum<std::size_t>(qual_length, 1);
                } else {
                    throw std::runtime_error("out-of-range quality score '" + std::string(1, val) + "' detected at line " + ritsuko::itoa(line_count));
                }
            }

            if (qual_length != seq_length) {
                throw std::runtime_error("unequal lengths for quality and sequence strings at line " + ritsuko::itoa(line_count));
            }
        }

        nseq = sanisizer::sum<std::size_t>(nseq, 1);
    }

    return nseq;
}

inline std::size_t parse_sequence_names(const std::filesystem::path& path, bool parallel) {
    auto gzreader = open_reader<byteme::GzipFileReader>(path, byteme::GzipFileReaderOptions());
    auto pb = wrap_reader_for_bytes<char>(std::move(gzreader), parallel);

    std::size_t nseq = 0;
    unsigned long long line_count = 1;
    auto advance_and_check = [&]() -> char {
        if (!pb->advance()) {
            throw std::runtime_error("premature end of the file at line " + ritsuko::itoa(line_count));
        }
        return pb->get();
    };

    while (pb->valid()) {
        char val = pb->get();
        if (val != '"') {
            throw std::runtime_error("name should start with a quote at line " + ritsuko::itoa(line_count));
        }

        while (true) {
            val = advance_and_check();
            if (val == '"') {
                val = advance_and_check();
                if (val == '\n') {
                    ++nseq;
                    ++line_count;
                    pb->advance();
                    break;
                } else if (val != '"') {
                    throw std::runtime_error("characters present after end quote at line " + ritsuko::itoa(line_count));
                }
            } else if (val == '\n') {
                ++line_count;
            }
        }
    }

    return nseq;
}
/**
 * @endcond
 */

/**
 * @param path Path to a directory containing a sequence string set.
 * @param metadata Metadata for the object, typically read from its `OBJECT` file.
 * @param options Validation options.
 */
inline void validate_sequence_string_set(const std::filesystem::path& path, const ObjectMetadata& metadata, const Options& options) {
    const std::string type_name = "sequence_string_set"; // use a separate variable to avoid dangling reference warnings from GCC.

    std::size_t expected_nseq = 0;
    AllowedCharacterMap allowed{};
    bool has_qualities = false;
    char lowest_quality = 0;

    try {
        const auto& obj = extract_json_object(metadata.other, type_name);
        try {
        const auto& vstring = extract_json_version_string(obj);
            auto version = ritsuko::parse_version_string(vstring.c_str(), vstring.size(), /* skip_patch = */ true);
            if (version.major != 1) {
                throw std::runtime_error("unsupported version string '" + vstring + "'");
            }
        } catch (...) {
            std::throw_with_nested(std::runtime_error("failed to read 'version'"));
        }

        try {
            auto lIt = obj.find("length");
            if (lIt == obj.end()) {
                throw std::runtime_error("no such property");
            }

            const auto& val = lIt->second;
            if (val->type() != millijson::NUMBER) {
                throw std::runtime_error("property should be a JSON number");
            }

            auto num = reinterpret_cast<const millijson::Number*>(val.get())->value();
            if (num < 0 || std::floor(num) != num) {
                throw std::runtime_error("property should be a non-negative integer");
            }

            expected_nseq = sanisizer::from_float<std::size_t>(num);
        } catch (...) {
            std::throw_with_nested(std::runtime_error("failed to read 'length'"));
        }

        try {
            const std::string stype_name = "sequence_type"; // avoid dangling reference again.
            const auto& stype = extract_json_string(obj, stype_name);

            std::string allowable;
            if (stype == "DNA" || stype == "RNA") {
                allowable = "ACGRYSWKMBDHVN";
                if (stype == "DNA") {
                    allowable += "T";
                } else {
                    allowable += "U";
                }
            } else if (stype == "AA") {
                allowable = "ACDEFGHIKLMNPQRSTVWY";
            } else if (stype == "custom") {
                std::fill(allowed.begin() + char2int('!'), allowed.begin() + char2int('~') + 1, true);
            } else {
                throw std::runtime_error("unknown sequence type '" + stype + "'");
            }

            const auto lower_shift = 'A' - 'a';
            for (auto a : allowable) {
                allowed[char2int(a)] = true;
                allowed[char2int(a) - lower_shift] = true; // don't trust std::tolower(), as this is locale dependent.
            }
            allowed[char2int('.')] = true;
            allowed[char2int('-')] = true;

        } catch (...) {
            std::throw_with_nested(std::runtime_error("failed to extract 'sequence_type'"));
        }

        bool offset_failed = false;
        try {
            auto xIt = obj.find("quality_type");
            if (xIt != obj.end()) {
                const auto& val = xIt->second;
                if (val->type() != millijson::STRING) {
                    throw std::runtime_error("expected a JSON string");
                }

                const auto& qtype = reinterpret_cast<const millijson::String*>(val.get())->value();

                if (qtype == "phred") {
                    has_qualities = true;
                    offset_failed = true;
                    try {
                        auto oIt = obj.find("quality_offset");
                        if (oIt == obj.end()) {
                            throw std::runtime_error("no such property");
                        }

                        const auto& val = oIt->second;
                        if (val->type() != millijson::NUMBER) {
                            throw std::runtime_error("property should be a JSON number");
                        }

                        double offset = reinterpret_cast<const millijson::Number*>(val.get())->value();
                        if (offset == 33) {
                            lowest_quality = '!';
                        } else if (offset == 64) {
                            lowest_quality = '@';
                        } else {
                            throw std::runtime_error("property should be either 33 or 64");
                        }
                    } catch (...) {
                        std::throw_with_nested(std::runtime_error("failed to extract 'quality_offset'"));
                    }

                } else if (qtype == "solexa") {
                    has_qualities = true;
                    lowest_quality = ';';

                } else if (qtype != "none") {
                    throw std::runtime_error("unknown quality type '" + qtype + "'");
                }
            }
        } catch (...) {
            if (offset_failed) {
                throw;
            } else {
                std::throw_with_nested(std::runtime_error("failed to extract 'quality_type'"));
            }
        }
    } catch (...) {
        std::throw_with_nested(std::runtime_error("failed to read '" + type_name + "' in the object metadata"));
    }

    std::string fname;
    try {
        std::size_t nseq = 0;
        if (has_qualities) {
            fname = "sequences.fastq.gz";
            nseq = parse_sequence_contents<true>(path / fname, allowed, lowest_quality, options.parallel_reads);
        } else {
            fname = "sequences.fasta.gz";
            nseq = parse_sequence_contents<false>(path / fname, allowed, lowest_quality, options.parallel_reads);
        }
        if (nseq != expected_nseq) {
            throw std::runtime_error("number of sequences is different from the expected length (" + ritsuko::itoa(nseq) + " versus " + ritsuko::itoa(expected_nseq) + ")");
        }
    } catch (...) {
        std::throw_with_nested(std::runtime_error("failed to validate '" + fname + "'"));
    }

    auto npath = path / "names.txt.gz";
    if (std::filesystem::exists(npath)) {
        try {
            std::size_t nnames = parse_sequence_names(npath, options.parallel_reads);
            if (nnames != expected_nseq) {
                throw std::runtime_error("number of names is different from the number of sequences (" + ritsuko::itoa(nnames) + " versus " + ritsuko::itoa(expected_nseq) + ")");
            }
        } catch (...) {
            std::throw_with_nested(std::runtime_error("failed to validate '" + npath.filename().string() + "'"));
        }
    }

    validate_mcols(path, "sequence_annotations", expected_nseq, options);
    validate_metadata(path, "other_annotations", options);
}

/**
 * @param path Path to a directory containing a sequence string set.
 * @param metadata Metadata for the object, typically read from its `OBJECT` file.
 * @param options Validation options.
 * @return The number of sequences.
 */
inline std::size_t height_of_sequence_string_set([[maybe_unused]] const std::filesystem::path& path, const ObjectMetadata& metadata, [[maybe_unused]] const Options& options) {
    const std::string type_name = "sequence_string_set"; // use a separate variable to avoid dangling reference warnings from GCC.
    const auto& obj = extract_json_object(metadata.other, type_name);
    auto lIt = obj.find("length");
    const auto& val = lIt->second;
    return sanisizer::from_float<std::size_t>(reinterpret_cast<const millijson::Number*>(val.get())->value());
}

}

#endif
