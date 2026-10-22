#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "utils.h"

#include <string>
#include <filesystem>
#include <vector>
#include <cstddef>

TEST(VcfExperiment, SimpleOkay) {
    auto dir = define_test_path("vcf_experiment");

    {
        initialize_directory(dir);
        quick_text_write(
            dir / "OBJECT",
            "{ \"type\": \"vcf_experiment\", \"vcf_experiment\": { \"version\": \"1.0\", \"dimensions\": [1, 2], \"expanded\": false } }"
        );
        quick_gzip_write(
            dir / "file.vcf.gz",
            "##fileformat=VCFv4\n"
            "##foobarbar\n"
            "#CHROM\tPOS\tID\tREF\tALT\tQUAL\tFILTER\tINFO\tFORMAT\tsam1\tsam2\n"
            "chr1\t1\tfoo\tA\tC\t10\tPASS\tNS=1\tGT\t1|0\t0|0\n"
        );
    }

    test_validate(dir);
    EXPECT_EQ(test_height(dir), 1); 
    EXPECT_EQ(test_dimensions(dir), (std::vector<std::size_t>{ 1, 2 })); 
}

/*********************************/

TEST(VcfExperiment, VersionError) {
    auto dir = define_test_path("vcf_experiment");

    {
        initialize_directory(dir);
        quick_text_write(
            dir / "OBJECT",
            "{ \"type\": \"vcf_experiment\", \"vcf_experiment\": { \"version\": \"2.0\" } }"
        );
    }
    expect_validation_error(dir, "unsupported version");
}

TEST(VcfExperiment, DimensionsError) {
    auto dir = define_test_path("vcf_experiment");

    // Check that the dimensions are actually validated.
    {
        initialize_directory(dir);
        quick_text_write(
            dir / "OBJECT",
            "{ \"type\": \"vcf_experiment\", \"vcf_experiment\": { \"version\": \"1.0\", \"dimensions\": true } }"
        );
    }
    expect_validation_error(dir, "an array");
}

TEST(VcfExperiment, ExpandedError) {
    auto dir = define_test_path("vcf_experiment");

    {
        initialize_directory(dir);
        quick_text_write(
            dir / "OBJECT",
            "{ \"type\": \"vcf_experiment\", \"vcf_experiment\": { \"version\": \"1.0\", \"dimensions\": [1, 2] } }"
        );
    }
    expect_validation_error(dir, "'expanded' property");

    {
        initialize_directory(dir);
        quick_text_write(
            dir / "OBJECT",
            "{ \"type\": \"vcf_experiment\", \"vcf_experiment\": { \"version\": \"1.0\", \"dimensions\": [1, 2], \"expanded\": 1 } }"
        );
    }
    expect_validation_error(dir, "JSON boolean");
}

/*********************************/

TEST(VcfExperiment, ParsingError) {
    auto dir = define_test_path("vcf_experiment");
    const std::string meta = "{ \"type\": \"vcf_experiment\", \"vcf_experiment\": { \"version\": \"1.0\", \"dimensions\": [1, 2], \"expanded\": false } }";

    {
        initialize_directory(dir);
        quick_text_write(dir / "OBJECT", meta);
        quick_gzip_write(dir / "file.vcf.gz", "##fileformat");
    }
    expect_validation_error(dir, "not enough bytes");

    {
        initialize_directory(dir);
        quick_text_write(dir / "OBJECT", meta);
        quick_gzip_write(dir / "file.vcf.gz", "##filefooomat");
    }
    expect_validation_error(dir, "incorrect signature for a VCF file");

    {
        initialize_directory(dir);
        quick_text_write(dir / "OBJECT", meta);
        quick_gzip_write(dir / "file.vcf.gz", "##fileformat=VCFv4\n");
    }
    expect_validation_error(dir, "premature end");

    {
        initialize_directory(dir);
        quick_text_write(dir / "OBJECT", meta);
        quick_gzip_write(dir / "file.vcf.gz", "##fileformat=VCFv4\n##aasdasd");
    }
    expect_validation_error(dir, "premature end");

    {
        initialize_directory(dir);
        quick_text_write(dir / "OBJECT", meta);
        quick_gzip_write(dir / "file.vcf.gz", "##fileformat=VCFv4\n##aasdasd\n");
    }
    expect_validation_error(dir, "premature end");

    {
        initialize_directory(dir);
        quick_text_write(dir / "OBJECT", meta);
        quick_gzip_write(dir / "file.vcf.gz", "##fileformat=VCFv4\n#CHROM");
    }
    expect_validation_error(dir, "premature end");

    {
        initialize_directory(dir);
        quick_text_write(dir / "OBJECT", meta);
        quick_gzip_write(
            dir / "file.vcf.gz",
            "##fileformat=VCFv4\n"
            "#CHROM\tPOS\tID\tREF\tALT\tQUAL\tFILTER\tINFO\tFORMAT\n"
            "chr1\t1\tfoo\tA\tC\t10\tPASS\tNS=1\tGT\n"
        );
    }
    expect_validation_error(dir, "does not match the number of samples");

    {
        initialize_directory(dir);
        quick_text_write(dir / "OBJECT", meta);
        quick_gzip_write(
            dir / "file.vcf.gz",
            "##fileformat=VCFv4\n"
            "#CHROM\tPOS\tID\tREF\tALT\tQUAL\tFILTER\tINFO\tFORMAT\tsam1\tsam2"
        );
    }
    expect_validation_error(dir, "premature end");

    {
        initialize_directory(dir);
        quick_text_write(dir / "OBJECT", meta);
        quick_gzip_write(
            dir / "file.vcf.gz",
            "##fileformat=VCFv4\n"
            "#CHROM\tREF\tALT\tQUAL\tFILTER\n"
        );
    }
    expect_validation_error(dir, "expected at least 9 fields");

    {
        initialize_directory(dir);
        quick_text_write(dir / "OBJECT", meta);
        quick_gzip_write(
            dir / "file.vcf.gz",
            "##fileformat=VCFv4\n"
            "##foobarbar\n"
            "#CHROM\tPOS\tID\tREF\tALT\tQUAL\tFILTER\tINFO\tFORMAT\tsam1\tsam2\n"
        );
    }
    expect_validation_error(dir, "does not match the number of records");
}

/*********************************/

TEST(VcfExperiment, CollapsedParsingOkay) {
    auto dir = define_test_path("vcf_experiment");

    {
        initialize_directory(dir);
        quick_text_write(
            dir / "OBJECT",
            "{ \"type\": \"vcf_experiment\", \"vcf_experiment\": { \"version\": \"1.0\", \"dimensions\": [4, 2], \"expanded\": false } }"
        );
        quick_gzip_write(
            dir / "file.vcf.gz",
            "##fileformat=VCFv4\n"
            "##aasdasd\n"
            "##foobarbar\n"
            "#CHROM\tPOS\tID\tREF\tALT\tQUAL\tFILTER\tINFO\tFORMAT\tsam1\tsam2\n"
            "chr1\t1\tfoo1\tA\tC\t10\tPASS\tNS=1\tGT\t1|0\t0|0\n"
            "chr1\t2\tfoo2\tA\tC,T\t10\tPASS\tNS=1\tGT\t1|0\t0|0\n"
            "chr1\t3\tfoo3\tA\t.\t10\tPASS\tNS=1\tGT\t1|0\t0|0\n"
            "chr1\t4\tfoo4\tAGGGG\tACTG,<DEL>,<MUL>\t10\tPASS\tNS=1\tGT\t1|0\t0|0\n"
        );
    }

    test_validate(dir);
    EXPECT_EQ(test_height(dir), 4); 
    EXPECT_EQ(test_dimensions(dir), (std::vector<std::size_t>{ 4, 2 })); 

    // FWIW we get the same result from parallel parsing.
    takane::Options inopt;
    inopt.parallel_reads = true;
    test_validate(dir, inopt);
}

TEST(VcfExperiment, CollapsedParsingError) {
    auto dir = define_test_path("vcf_experiment");

    std::string meta = "{ \"type\": \"vcf_experiment\", \"vcf_experiment\": { \"version\": \"1.0\", \"dimensions\": [4, 2], \"expanded\": false } }";
    std::string contents =
        "##fileformat=VCFv4\n"
        "##aasdasd\n"
        "##foobarbar\n"
        "#CHROM\tPOS\tID\tREF\tALT\tQUAL\tFILTER\tINFO\tFORMAT\tsam1\tsam2\n"
        "chr1\t1\tfoo1\tA\tC\t10\tPASS\tNS=1\tGT\t1|0\t0|0\n"
        "chr1\t2\tfoo2\tA\tC,T\t10\tPASS\tNS=1\tGT\t1|0\t0|0\n"
    ;

    {
        initialize_directory(dir);
        quick_text_write(dir / "OBJECT", meta);
        quick_gzip_write(dir / "file.vcf.gz", contents);
    }
    expect_validation_error(dir, "does not match the number of records");

    // No terminating newline.
    {
        initialize_directory(dir);
        quick_text_write(dir / "OBJECT", meta);
        auto contents2 = contents + 
            "chr1\t3\tfoo3\tA\t.\t10\tPASS\tNS=1\tGT\t1|0\t0|0\n"
            "chr1\t4\tfoo4\tAGGGG\tACTG,<DEL>,<MUL>\t10\tPASS\tNS=1\tGT\t1|0\t0|0"
        ;
        quick_gzip_write(dir / "file.vcf.gz", contents2);
    }
    expect_validation_error(dir, "premature end");
}

/*********************************/

TEST(VcfExperiment, ExpandedParsingOkay) {
    auto dir = define_test_path("vcf_experiment");

    {
        initialize_directory(dir);
        quick_text_write(
            dir / "OBJECT",
            "{ \"type\": \"vcf_experiment\", \"vcf_experiment\": { \"version\": \"1.0\", \"dimensions\": [4, 2], \"expanded\": true } }"
        );
        quick_gzip_write(
            dir / "file.vcf.gz",
            "##fileformat=VCFv4\n"
            "##aasdasd\n"
            "##foobarbar\n"
            "#CHROM\tPOS\tID\tREF\tALT\tQUAL\tFILTER\tINFO\tFORMAT\tsam1\tsam2\n"
            "chr1\t1\tfoo1\tA\tC\t10\tPASS\tNS=1\tGT\t1|0\t0|0\n"
            "chr1\t2\tfoo2\tA\tC\t10\tPASS\tNS=1\tGT\t1|0\t0|0\n"
            "chr1\t3\tfoo3\tA\t.\t10\tPASS\tNS=1\tGT\t1|0\t0|0\n"
            "chr1\t4\tfoo4\tAGGGG\t<DEL>\t10\tPASS\tNS=1\tGT\t1|0\t0|0\n"
        );
    }

    test_validate(dir);
    EXPECT_EQ(test_height(dir), 4);
    EXPECT_EQ(test_dimensions(dir), (std::vector<std::size_t>{ 4, 2 }));
}

TEST(VcfExperiment, ExpandedParsingError) {
    auto dir = define_test_path("vcf_experiment");

    std::string contents =
        "##fileformat=VCFv4\n"
        "##aasdasd\n"
        "##foobarbar\n"
        "#CHROM\tPOS\tID\tREF\tALT\tQUAL\tFILTER\tINFO\tFORMAT\tsam1\tsam2\n"
        "chr1\t1\tfoo1\tA\tC\t10\tPASS\tNS=1\tGT\t1|0\t0|0\n"
    ;
    const std::string meta = "{ \"type\": \"vcf_experiment\", \"vcf_experiment\": { \"version\": \"1.0\", \"dimensions\": [4, 2], \"expanded\": true } }";

    {
        initialize_directory(dir);
        quick_text_write(dir / "OBJECT", meta);
        quick_gzip_write(dir / "file.vcf.gz", contents + "chr1\t2\tfoo2");
    }
    expect_validation_error(dir, "premature end");

    {
        initialize_directory(dir);
        quick_text_write(dir / "OBJECT", meta);
        quick_gzip_write(dir / "file.vcf.gz", contents + "chr1\t2\tfoo2\n");
    }
    expect_validation_error(dir, "premature end");

    {
        initialize_directory(dir);
        quick_text_write(dir / "OBJECT", meta);
        quick_gzip_write(dir / "file.vcf.gz", contents + "chr1\t2\tfoo2\tA\t");
    }
    expect_validation_error(dir, "premature end");

    {
        initialize_directory(dir);
        quick_text_write(dir / "OBJECT", meta);
        quick_gzip_write(dir / "file.vcf.gz", contents + "chr1\t2\tfoo2\tA\tC");
    }
    expect_validation_error(dir, "premature end");

    {
        initialize_directory(dir);
        quick_text_write(dir / "OBJECT", meta);
        quick_gzip_write(dir / "file.vcf.gz", contents + "chr1\t2\tfoo2\tA\tC,T");
    }
    expect_validation_error(dir, "expected a 1:1 mapping");

    {
        initialize_directory(dir);
        quick_text_write(dir / "OBJECT", meta);
        quick_gzip_write(dir / "file.vcf.gz", contents + "chr1\t2\tfoo2\tA\tC\n");
    }
    expect_validation_error(dir, "premature end");

    {
        initialize_directory(dir);
        quick_text_write(dir / "OBJECT", meta);
        quick_gzip_write(dir / "file.vcf.gz", contents + "chr1\t2\tfoo2\tA\tC\t");
    }
    expect_validation_error(dir, "premature end");

    {
        initialize_directory(dir);
        quick_text_write(dir / "OBJECT", meta);
        quick_gzip_write(
            dir / "file.vcf.gz",
            contents +
                "chr1\t2\tfoo2\tA\tC\t10\tPASS\tNS=1\tGT\t1|0\t0|0\n"
                "chr1\t3\tfoo3\tA\t.\t10\tPASS\tNS=1\tGT\t1|0\t0|0\n"
                "chr1\t4\tfoo4\tAGGGG\t<DEL>\t10\tPASS\tNS=1\tGT\t1|0\t0|0"
        );
    }
    expect_validation_error(dir, "premature end");
}
