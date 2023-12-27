#ifndef SEQUENCE_STRING_SET_H 
#define SEQUENCE_STRING_SET_H

#include <vector>
#include <string>
#include <numeric>

#include "H5Cpp.h"
#include "utils.h"

#include "byteme/byteme.hpp"

namespace sequence_string_set {

enum class SequenceType : char { DNA, RNA, AA, CUSTOM };

enum class QualityType : char { NONE, PHRED33, SOLEXA, ILLUMINA64 };

struct Options {
    SequenceType sequence_type = SequenceType::DNA;
    QualityType quality_type = QualityType::NONE;
};

inline void dump_fasta(byteme::GzipFileWriter& writer, size_t i, const std::string& seq) {
    writer.write(">");
    writer.write(std::to_string(i));
    writer.write("\n");
    writer.write(seq);
    writer.write("\n");
}

inline void dump_fastq(byteme::GzipFileWriter& writer, size_t i, const std::string& seq, const std::string& qual) {
    writer.write("@");
    writer.write(std::to_string(i));
    writer.write("\n");
    writer.write(seq);
    writer.write("\n+\n");
    writer.write(qual);
    writer.write("\n");
}

inline void mock(const std::filesystem::path& dir, size_t length, const Options& options) {
    initialize_directory(dir);

    std::ofstream handle(dir / "OBJECT");
    handle << "{ \"type\": \"sequence_string_set\", \"sequence_string_set\": { \"version\": \"1.0\"";
    handle << ", \"length\": " << length;

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

    if (options.quality_type != QualityType::NONE) {
        auto spath = dir / "sequences.fastq.gz";
        byteme::GzipFileWriter writer(spath.c_str());
        for (size_t i = 0; i < length; ++i) {
            dump_fastq(writer, i, "AAAAAA", "EEEEEE");
        }
    } else {
        auto spath = dir / "sequences.fasta.gz";
        byteme::GzipFileWriter writer(spath.c_str());
        for (size_t i = 0; i < length; ++i) {
            dump_fasta(writer, i, "AAAAAA");
        }
    }
}

}

#endif
