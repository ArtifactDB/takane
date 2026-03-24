#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/vcf_experiment.hpp"
#include "utils.h"
#include "data_frame.h"
#include "simple_list.h"

#include <string>
#include <filesystem>
#include <vector>
#include <cstddef>

struct VcfExperimentTest : public ::testing::Test {
    VcfExperimentTest() {
        dir = "TEST_vcf_experiment";
        name = "vcf_experiment";
    }

    std::filesystem::path dir;
    std::string name;

    template<typename ... Args_>
    void expect_error(const std::string& msg, Args_&& ... args) {
        EXPECT_ANY_THROW({
            try {
                test_validate(dir, std::forward<Args_>(args)...);
            } catch (std::exception& e) {
                EXPECT_THAT(e.what(), ::testing::HasSubstr(msg));
                throw;
            }
        });
    }
};

TEST_F(VcfExperimentTest, MetadataRetrieval) {
    initialize_directory(dir);
    auto objpath = (dir / "OBJECT").string();

    quick_text_write(objpath, 
        "{ \"type\": \"vcf_experiment\", \"vcf_experiment\": { \"version\": \"2.0\" } }"
    );
    expect_error("unsupported version");

    quick_text_write(objpath,
        "{ \"type\": \"vcf_experiment\", \"vcf_experiment\": { \"version\": \"1.0\" } }"
    );
    expect_error("expected a 'vcf_experiment.dimensions' property");

    quick_text_write(objpath,
        "{ \"type\": \"vcf_experiment\", \"vcf_experiment\": { \"version\": \"1.0\", \"dimensions\": true } }"
    );
    expect_error("an array");

    quick_text_write(objpath,
        "{ \"type\": \"vcf_experiment\", \"vcf_experiment\": { \"version\": \"1.0\", \"dimensions\": [1, 2] } }"
    );
    expect_error("vcf_experiment.expanded");

    quick_text_write(objpath,
        "{ \"type\": \"vcf_experiment\", \"vcf_experiment\": { \"version\": \"1.0\", \"dimensions\": [1, 2], \"expanded\": 1 } }"
    );
    expect_error("JSON boolean");
}

TEST_F(VcfExperimentTest, BasicParsing) {
    auto vpath = (dir / "file.vcf.gz").string();

    quick_text_write((dir / "OBJECT").string(),
        "{ \"type\": \"vcf_experiment\", \"vcf_experiment\": { \"version\": \"1.0\", \"dimensions\": [1, 2], \"expanded\": false } }"
    );
    quick_gzip_write(vpath, "##fileformat");
    expect_error("incomplete VCF file signature");

    quick_gzip_write(vpath, "##filefooomat");
    expect_error("incorrect VCF file signature");

    quick_gzip_write(vpath, "##fileformat=VCFv4\n");
    expect_error("premature end");

    quick_gzip_write(vpath, "##fileformat=VCFv4\n##aasdasd");
    expect_error("premature end");

    quick_gzip_write(vpath, "##fileformat=VCFv4\n##aasdasd\n");
    expect_error("premature end");

    quick_gzip_write(vpath, "##fileformat=VCFv4\n#CHROM");
    expect_error("premature end");

    quick_gzip_write(vpath,
        "##fileformat=VCFv4\n"
        "#CHROM\tPOS\tID\tREF\tALT\tQUAL\tFILTER\tINFO\tFORMAT\n"
        "chr1\t1\tfoo\tA\tC\t10\tPASS\tNS=1\tGT\n"
    );
    expect_error("does not match the number of samples");

    quick_gzip_write(vpath, 
        "##fileformat=VCFv4\n"
        "#CHROM\tPOS\tID\tREF\tALT\tQUAL\tFILTER\tINFO\tFORMAT\tsam1\tsam2"
    );
    expect_error("premature end");

    quick_gzip_write(vpath, 
        "##fileformat=VCFv4\n"
        "#CHROM\tREF\tALT\tQUAL\tFILTER\n"
    );
    expect_error("expected at least 9 fields");

    quick_gzip_write(vpath,
        "##fileformat=VCFv4\n"
        "##foobarbar\n"
        "#CHROM\tPOS\tID\tREF\tALT\tQUAL\tFILTER\tINFO\tFORMAT\tsam1\tsam2\n"
    );
    expect_error("does not match the number of records");

    quick_gzip_write(vpath,
        "##fileformat=VCFv4\n"
        "##foobarbar\n"
        "#CHROM\tPOS\tID\tREF\tALT\tQUAL\tFILTER\tINFO\tFORMAT\tsam1\tsam2\n"
        "chr1\t1\tfoo\tA\tC\t10\tPASS\tNS=1\tGT\t1|0\t0|0\n"
    );
    test_validate(dir);
}

TEST_F(VcfExperimentTest, CollapsedParsing) {
    quick_text_write((dir / "OBJECT").string(),
        "{ \"type\": \"vcf_experiment\", \"vcf_experiment\": { \"version\": \"1.0\", \"dimensions\": [4, 2], \"expanded\": false } }"
    );

    std::string contents = "##fileformat=VCFv4\n";
    contents += "##aasdasd\n";
    contents += "##foobarbar\n";
    contents += "#CHROM\tPOS\tID\tREF\tALT\tQUAL\tFILTER\tINFO\tFORMAT\tsam1\tsam2\n";
    contents += "chr1\t1\tfoo1\tA\tC\t10\tPASS\tNS=1\tGT\t1|0\t0|0\n";
    contents += "chr1\t2\tfoo2\tA\tC,T\t10\tPASS\tNS=1\tGT\t1|0\t0|0\n";

    auto vpath = (dir / "file.vcf.gz").string();
    quick_gzip_write(vpath, contents);
    expect_error("does not match the number of records");

    contents += "chr1\t3\tfoo3\tA\t.\t10\tPASS\tNS=1\tGT\t1|0\t0|0\n";
    contents += "chr1\t4\tfoo4\tAGGGG\tACTG,<DEL>,<MUL>\t10\tPASS\tNS=1\tGT\t1|0\t0|0";
    quick_gzip_write(vpath, contents);
    expect_error("premature end");

    contents += "\n";
    quick_gzip_write(vpath, contents);
    test_validate(dir);

    EXPECT_EQ(test_height(dir), 4);
    std::vector<std::size_t> expected_dims { 4, 2 };
    EXPECT_EQ(test_dimensions(dir), expected_dims);

    // FWIW we get the same result from parallel parsing.
    auto meta = takane::read_object_metadata(dir);
    takane::Options inopt;
    inopt.parallel_reads = true;
    takane::vcf_experiment::validate(dir, meta, inopt);
}

TEST_F(VcfExperimentTest, ExpandedParsing) {
    quick_text_write((dir / "OBJECT").string(),
        "{ \"type\": \"vcf_experiment\", \"vcf_experiment\": { \"version\": \"1.0\", \"dimensions\": [4, 2], \"expanded\": true } }"
    );

    std::string contents = "##fileformat=VCFv4\n";
    contents += "##aasdasd\n";
    contents += "##foobarbar\n";
    contents += "#CHROM\tPOS\tID\tREF\tALT\tQUAL\tFILTER\tINFO\tFORMAT\tsam1\tsam2\n";
    contents += "chr1\t1\tfoo1\tA\tC\t10\tPASS\tNS=1\tGT\t1|0\t0|0\n";

    auto vpath = (dir / "file.vcf.gz").string();
    quick_gzip_write(vpath, contents + "chr1\t2\tfoo2");
    expect_error("premature end");

    quick_gzip_write(vpath, contents + "chr1\t2\tfoo2\n");
    expect_error("premature end");

    quick_gzip_write(vpath, contents + "chr1\t2\tfoo2\tA\t");
    expect_error("premature end");

    quick_gzip_write(vpath, contents + "chr1\t2\tfoo2\tA\tC");
    expect_error("premature end");

    quick_gzip_write(vpath, contents + "chr1\t2\tfoo2\tA\tC,T");
    expect_error("expected a 1:1 mapping");

    quick_gzip_write(vpath, contents + "chr1\t2\tfoo2\tA\tC\n");
    expect_error("premature end");

    quick_gzip_write(vpath, contents + "chr1\t2\tfoo2\tA\tC\t");
    expect_error("premature end");

    contents += "chr1\t2\tfoo2\tA\tC\t10\tPASS\tNS=1\tGT\t1|0\t0|0\n";
    contents += "chr1\t3\tfoo3\tA\t.\t10\tPASS\tNS=1\tGT\t1|0\t0|0\n";
    contents += "chr1\t4\tfoo4\tAGGGG\t<DEL>\t10\tPASS\tNS=1\tGT\t1|0\t0|0";
    quick_gzip_write(vpath, contents);
    expect_error("premature end");

    contents += "\n";
    quick_gzip_write(vpath, contents);
    test_validate(dir);

    EXPECT_EQ(test_height(dir), 4);
    std::vector<std::size_t> expected_dims { 4, 2 };
    EXPECT_EQ(test_dimensions(dir), expected_dims);
}
