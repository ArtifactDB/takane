#ifndef MOCK_SEQUENCE_STRING_SET_H 
#define MOCK_SEQUENCE_STRING_SET_H

#include <vector>
#include <string>
#include <numeric>
#include <fstream>

#include "H5Cpp.h"
#include "utils.h"

#include "byteme/byteme.hpp"

enum class SequenceType : char { DNA, RNA, AA, CUSTOM };

enum class QualityType : char { NONE, PHRED33, SOLEXA, ILLUMINA64 };

struct SequenceStringSetOptions {
    SequenceStringSetOptions(std::size_t length_arg) : length(length_arg), version("1.0") {}
    std::size_t length;
    SequenceType sequence_type = SequenceType::DNA;
    QualityType quality_type = QualityType::NONE;
    std::string version;
};

inline void dump_fasta(byteme::GzipFileWriter& writer, std::size_t i, const std::string& seq) {
    std::string record = ">" + std::to_string(i) + "\n" + seq + "\n";
    writer.write(reinterpret_cast<const unsigned char*>(record.c_str()), record.size());
}

inline void dump_fastq(byteme::GzipFileWriter& writer, std::size_t i, const std::string& seq, const std::string& qual) {
    std::string record = "@" + std::to_string(i) + "\n" + seq + "\n+\n" + qual + "\n";
    writer.write(reinterpret_cast<const unsigned char*>(record.c_str()), record.size());
}

inline void initialize_sequence_string_set(const std::filesystem::path& dir, const SequenceStringSetOptions& options) {
    initialize_directory(dir);

    std::ofstream handle(dir / "OBJECT");
    handle << "{ \"type\": \"sequence_string_set\", \"sequence_string_set\": ";
    handle << "{ \"version\": \"" << options.version << "\"";
    handle << ", \"length\": " << options.length;

    handle << ", \"sequence_type\": \"";
    switch (options.sequence_type) {
        case SequenceType::RNA:
            handle << "RNA";
            break;
        case SequenceType::DNA:
            handle << "DNA";
            break;
        case SequenceType::AA:
            handle << "AA";
            break;
        case SequenceType::CUSTOM:
            handle << "custom";
            break;
    }
    handle << "\"";

    if (options.quality_type != QualityType::NONE) {
        int offset = 0;
        handle << ", \"quality_type\": \"";
        switch (options.quality_type) {
            case QualityType::PHRED33:
                handle << "phred";
                offset = 33;
                break;
            case QualityType::SOLEXA:
                handle << "solexa";
                break;
            default: // i.e., QualityType::ILLUMINA64:
                handle << "phred";
                offset = 64;
                break;
        }
        handle << "\"";
        if (offset != 0) {
            handle << ", \"quality_offset\": " << offset;
        }
    }
    handle << " } }\n";
}

inline void mock_sequence_string_set(const std::filesystem::path& dir, const SequenceStringSetOptions& options) {
    initialize_sequence_string_set(dir, options);

    if (options.quality_type != QualityType::NONE) {
        auto spath = dir / "sequences.fastq.gz";
        byteme::GzipFileWriter writer(spath.c_str(), {});
        for (std::size_t i = 0; i < options.length; ++i) {
            dump_fastq(writer, i, "AAAAAA", "EEEEEE");
        }
    } else {
        auto spath = dir / "sequences.fasta.gz";
        byteme::GzipFileWriter writer(spath.c_str(), {});
        for (std::size_t i = 0; i < options.length; ++i) {
            dump_fasta(writer, i, "AAAAAA");
        }
    }
}

#endif
