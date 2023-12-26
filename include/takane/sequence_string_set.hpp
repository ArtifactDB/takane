#ifndef TAKANE_SEQUENCE_STRING_SET_HPP
#define TAKANE_SEQUENCE_STRING_SET_HPP

#include "byteme/byteme.hpp"

#include <array>
#include <algorithm>
#include <stdexcept>
#include <limits>
#include <cctype>

/**
 * @file sequence_string_set.hpp
 * @brief Validation for sequence string sets.
 */

namespace takane {

/**
 * @namespace takane::sequence_string_set
 * @brief Definitions for sequence string sets.
 */
namespace sequence_string_set {

/**
 * @cond
 */
namespace internal {

inline int char2int(char val) {
    return static_cast<int>(val) - static_cast<int>(std::numeric_limits<char>::min());
}

template<bool has_quality_, bool parallel_>
size_t parse_sequences(const std::filesystem::path& path, std::array<bool, 255> allowed, int lowest_quality) {
    auto gzreader = internal_other::open_reader<byteme::GzipFileReader>(path);
    typedef typename std::conditional<parallel_, byteme::PerByteParallel<>, byteme::PerByte<> >::type PB;
    PB pb(&gzreader);

    size_t nseq = 0;
    size_t line_count = 0;

    auto advance_and_check = [&]() -> char {
        if (!pb.advance()) {
            throw std::runtime_error("premature end of the file at line " + std::to_string(line_count + 1));
        }
        return pb.get();
    };

    while (pb.valid()) {
        // Processing the name. 
        char val = pb.get();
        if constexpr(!has_quality_) {
            if (val != '>') {
                throw std::runtime_error("sequence name should start with '>' at line " + std::to_string(line_count + 1));
            }
        } else {
            if (val != '@') {
                throw std::runtime_error("sequence name should start with '@' at line " + std::to_string(line_count + 1));
            }
        }

        val = advance_and_check();
        size_t proposed = 0;
        bool empty = true;
        while (val != '\n') {
            if (!std::isdigit(val)) {
                throw std::runtime_error("sequence name should be a non-negative integer at line " + std::to_string(line_count + 1));
            }
            empty = false;
            proposed += val - '0';
            val = advance_and_check();
        }
        if (empty || proposed != nseq) {
            throw std::runtime_error("sequence name should be its index at line " + std::to_string(line_count + 1));
        }
        ++line_count;

        if constexpr(!has_quality_) {
            // Processing the sequence itself until we get to a '>' or we run out of bytes.
            val = advance_and_check();
            while (true) {
                if (val == '\n') {
                    ++line_count;
                    if (!pb.advance()) {
                        break;
                    }
                    val = pb.get();
                    if (val == '>') {
                        break;
                    }
                } else {
                    if (!allowed[char2int(val)]) {
                        throw std::runtime_error("forbidden character '" + std::string(1, val) + "' in sequence at line " + std::to_string(line_count + 1));
                    }
                    val = advance_and_check();
                }
            }

        } else {
            // Processing the sequence itself until we get to a '+'.
            val = advance_and_check();
            size_t seq_length = 0;
            while (true) {
                if (val == '\n') {
                    ++line_count;
                    val = advance_and_check();
                    if (val == '+') {
                        break;
                    }
                } else {
                    if (!allowed[char2int(val)]) {
                        throw std::runtime_error("forbidden character '" + std::string(1, val) + "' in ASCII-encoded qualities at line " + std::to_string(line_count + 1));
                    }
                    ++seq_length;
                    val = advance_and_check();
                }
            }

            // Next should be a single line; starting with '+' is implicit from above.
            do {
                val = advance_and_check();
            } while (val != '\n');
            ++line_count;

            // Processing the qualities. Extraction is allowed to fail if we're at
            // the end of the file. Note that we can't check for '@' as a
            // delimitor, as this can be a valid score, so instead we check at each
            // newline whether we've reached the specified length, and quit if so.
            size_t qual_length = 0;
            while (true) {
                val = advance_and_check();
                if (val == '\n') {
                    ++line_count;
                    if (qual_length >= seq_length) {
                        pb.advance(); // sneak past the newline.
                        break;
                    }
                } else {
                    if (char2int(val) < lowest_quality) {
                        throw std::runtime_error("out-of-range quality score '" + std::string(1, val) + "' detected at line " + std::to_string(line_count + 1));
                    }
                    ++qual_length;
                }
            }

            if (qual_length != seq_length) {
                throw std::runtime_error("non-equal lengths for quality and sequence strings at line " + std::to_string(line_count + 1) + ")");
            }
        }

        ++nseq;
    }

    return nseq;
}

}
/**
 * @endcond
 */

/**
 * @param path Path to a directory containing a sequence string set.
 * @param metadata Metadata for the object, typically read from its `OBJECT` file.
 * @param options Validation options, mostly for input performance.
 */
inline void validate(const std::filesystem::path& path, const ObjectMetadata& metadata, const Options& options) {
    const auto& obj = internal_json::extract_typed_object_from_metadata(metadata.other, "sequence_string_set");
    const auto& vstring = internal_json::extract_string_from_typed_object(obj, "version", "sequence_string_set");
    auto version = ritsuko::parse_version_string(vstring.c_str(), vstring.size(), /* skip_patch = */ true);
    if (version.major != 1) {
        throw std::runtime_error("unsupported version string '" + vstring + "'");
    }

    size_t expected_nseq = 0;
    {
        auto lIt = obj.find("length");
        if (lIt == obj.end()) {
            throw std::runtime_error("expected a 'sequence_string_set.length' property");
        }

        const auto& val = lIt->second;
        if (val->type() != millijson::NUMBER) {
            throw std::runtime_error("'sequence_string_set.length' property should be a JSON number");
        }

        auto num = reinterpret_cast<const millijson::Number*>(val.get())->value;
        if (num < 0 || std::floor(num) != num) {
            throw std::runtime_error("'sequence_string_set.length' should be a non-negative integer");
        }

        expected_nseq = num;
    }

    std::array<bool, 255> allowed;
    std::fill(allowed.begin(), allowed.end(), false);
    {
        const std::string& stype = internal_json::extract_string(obj, "sequence_type", [&](std::exception& e) -> void {
            throw std::runtime_error("failed to extract 'sequence_string_set.sequence_type' from the object metadata; " + std::string(e.what())); 
        });

        std::string allowable;
        if (stype == "DNA" || stype == "RNA") {
            allowable = "ACGRYSWKMBDHVN.-";
            if (stype == "DNA") {
                allowable += "T";
            } else {
                allowable += "U";
            }
        } else if (stype == "AA") {
            allowable = "ACDEFGHIKLMNPQRSTVWY";
        } else if (stype == "custom") {
            std::fill(allowed.begin() + 33, allowed.begin() + 127, true);
        }

        for (auto a : allowable) {
            allowed[internal::char2int(a)] = true;
            allowed[internal::char2int(std::tolower(a))] = true;
        }
    }

    bool has_qualities = false;
    int lowest_quality = 0;
    {
        auto xIt = obj.find("quality_type");
        if (xIt != obj.end()) {
            const auto& val = xIt->second;
            if (val->type() != millijson::STRING) {
                throw std::runtime_error("'sequence_string_set.quality_type' property should be a JSON string");
            }

            const auto& qtype = reinterpret_cast<const millijson::String*>(val.get())->value;
            if (qtype == "phred") {
                has_qualities = true;

                auto oIt = obj.find("quality_offset");
                if (oIt == obj.end()) {
                    throw std::runtime_error("expected a 'sequence_string_set.quality_offset' property for Phred quality scores");
                }

                const auto& val = oIt->second;
                if (val->type() != millijson::NUMBER) {
                    throw std::runtime_error("'sequence_string_set.quality_offset' property should be a JSON number");
                }

                double offset = reinterpret_cast<const millijson::Number*>(val.get())->value;
                if (offset != 33 && offset != 64) {
                    throw std::runtime_error("'sequence_string_set.quality_offset' property should be either 33 or 64");
                }
                lowest_quality = offset;

            } else if (qtype == "solexa") {
                lowest_quality = 64 - 5;

            } else if (qtype != "none") {
                throw std::runtime_error("invalid string '" + qtype + "' for the 'sequence_string_set.quality_type' property");
            }
        }
    }

    size_t nseq = 0;
    auto spath = path / "sequences.fastq.gz";
    if (has_qualities) {
        if (options.parallel_reads) {
            nseq = internal::parse_sequences<true, true>(spath, allowed, lowest_quality);
        } else {
            nseq = internal::parse_sequences<true, false>(spath, allowed, lowest_quality);
        }
    } else {
        if (options.parallel_reads) {
            nseq = internal::parse_sequences<false, true>(spath, allowed, lowest_quality);
        } else {
            nseq = internal::parse_sequences<false, false>(spath, allowed, lowest_quality);
        }
    }
    if (nseq != expected_nseq) {
        throw std::runtime_error("observed number of sequences is different from the expected number (" + std::to_string(nseq) + " to " + std::to_string(expected_nseq) + ")");
    }

    internal_other::validate_mcols(path, "sequence_data", nseq, options);
    internal_other::validate_metadata(path, "other_data", options);
}

/**
 * @param path Path to a directory containing a sequence string set.
 * @param metadata Metadata for the object, typically read from its `OBJECT` file.
 * @param options Validation options, mostly for input performance.
 * @return The number of sequences.
 */
inline size_t height([[maybe_unused]] const std::filesystem::path& path, const ObjectMetadata& metadata, [[maybe_unused]] const Options& options) {
    const auto& obj = internal_json::extract_typed_object_from_metadata(metadata.other, "sequence_string_set");
    auto lIt = obj.find("length");
    const auto& val = lIt->second;
    return reinterpret_cast<const millijson::Number*>(val.get())->value;
}

}

}

#endif
