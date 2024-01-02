#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/vcf_experiment.hpp"
#include "utils.h"
#include "data_frame.h"
#include "simple_list.h"

#include <fstream>
#include <string>

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
    {
        std::ofstream handle(dir / "OBJECT");
        handle << "{ \"type\": \"vcf_experiment\", \"vcf_experiment\": { \"version\": \"2.0\" } }";
    }
    expect_error("unsupported version");

    {
        std::ofstream handle(dir / "OBJECT");
        handle << "{ \"type\": \"vcf_experiment\", \"vcf_experiment\": { \"version\": \"1.0\" } }";
    }
    expect_error("expected a 'vcf_experiment.dimensions' property");

    {
        std::ofstream handle(dir / "OBJECT");
        handle << "{ \"type\": \"vcf_experiment\", \"vcf_experiment\": { \"version\": \"1.0\", \"dimensions\": true } }";
    }
    expect_error("an array");

    {
        std::ofstream handle(dir / "OBJECT");
        handle << "{ \"type\": \"vcf_experiment\", \"vcf_experiment\": { \"version\": \"1.0\", \"dimensions\": [1, 2] } }";
    }
    expect_error("vcf_experiment.expanded");

    {
        std::ofstream handle(dir / "OBJECT");
        handle << "{ \"type\": \"vcf_experiment\", \"vcf_experiment\": { \"version\": \"1.0\", \"dimensions\": [1, 2], \"expanded\": 1 } }";
    }
    expect_error("JSON boolean");
}

TEST_F(VcfExperimentTest, BasicParsing) {
    {
        std::ofstream handle(dir / "OBJECT");
        handle << "{ \"type\": \"vcf_experiment\", \"vcf_experiment\": { \"version\": \"1.0\", \"dimensions\": [1, 2], \"expanded\": false } }";
        byteme::GzipFileWriter writer(dir / "file.vcf.gz");
        writer.write("##fileformat");
    }
    expect_error("incomplete VCF file signature");

    {
        byteme::GzipFileWriter writer(dir / "file.vcf.gz");
        writer.write("##filefooomat");
    }
    expect_error("incorrect VCF file signature");

    std::string contents; 
    {
        byteme::GzipFileWriter writer(dir / "file.vcf.gz");
        contents = "##fileformat=VCFv4\n";
        writer.write(contents);
    }
    expect_error("premature end");

    {
        byteme::GzipFileWriter writer(dir / "file.vcf.gz");
        writer.write(contents);
        writer.write("##aasdasd");
    }
    expect_error("premature end");

    {
        byteme::GzipFileWriter writer(dir / "file.vcf.gz");
        contents += "##aasdasd\n";
        writer.write(contents);
    }
    expect_error("premature end");

    {
        byteme::GzipFileWriter writer(dir / "file.vcf.gz");
        writer.write(contents);
        writer.write("#CHROM");
    }
    expect_error("premature end");

    {
        byteme::GzipFileWriter writer(dir / "file.vcf.gz");
        writer.write(contents);
        writer.write("#CHROM\tPOS\tID\tREF\tALT\tQUAL\tFILTER\tINFO\tFORMAT\n");
        writer.write("chr1\t1\tfoo\tA\tC\t10\tPASS\tNS=1\tGT\n");
    }
    expect_error("does not match the number of samples");

    {
        byteme::GzipFileWriter writer(dir / "file.vcf.gz");
        writer.write(contents);
        writer.write("#CHROM\tPOS\tID\tREF\tALT\tQUAL\tFILTER\tINFO\tFORMAT\tsam1\tsam2");
    }
    expect_error("premature end");

    {
        byteme::GzipFileWriter writer(dir / "file.vcf.gz");
        writer.write(contents);
        writer.write("#CHROM\tREF\tALT\tQUAL\tFILTER\n");
    }
    expect_error("expected at least 9 fields");

    {
        byteme::GzipFileWriter writer(dir / "file.vcf.gz");
        contents += "##foobarbar\n";
        contents += "#CHROM\tPOS\tID\tREF\tALT\tQUAL\tFILTER\tINFO\tFORMAT\tsam1\tsam2\n";
        writer.write(contents);
    }
    expect_error("does not match the number of records");

    {
        byteme::GzipFileWriter writer(dir / "file.vcf.gz");
        writer.write(contents);
        writer.write("chr1\t1\tfoo\tA\tC\t10\tPASS\tNS=1\tGT\t1|0\t0|0\n");
    }
    test_validate(dir);
}

TEST_F(VcfExperimentTest, CollapsedParsing) {
    std::string contents = "##fileformat=VCFv4\n";
    contents += "##aasdasd\n";
    contents += "##foobarbar\n";
    contents += "#CHROM\tPOS\tID\tREF\tALT\tQUAL\tFILTER\tINFO\tFORMAT\tsam1\tsam2\n";
    contents += "chr1\t1\tfoo1\tA\tC\t10\tPASS\tNS=1\tGT\t1|0\t0|0\n";
    contents += "chr1\t2\tfoo2\tA\tC,T\t10\tPASS\tNS=1\tGT\t1|0\t0|0\n";

    {
        std::ofstream handle(dir / "OBJECT");
        handle << "{ \"type\": \"vcf_experiment\", \"vcf_experiment\": { \"version\": \"1.0\", \"dimensions\": [4, 2], \"expanded\": false } }";

        byteme::GzipFileWriter writer(dir / "file.vcf.gz");
        writer.write(contents);
    }
    expect_error("does not match the number of records");

    contents += "chr1\t3\tfoo3\tA\t.\t10\tPASS\tNS=1\tGT\t1|0\t0|0\n";
    contents += "chr1\t4\tfoo4\tAGGGG\tACTG,<DEL>,<MUL>\t10\tPASS\tNS=1\tGT\t1|0\t0|0";
    {
        byteme::GzipFileWriter writer(dir / "file.vcf.gz");
        writer.write(contents);
    }
    expect_error("premature end");

    contents += "\n";
    {
        byteme::GzipFileWriter writer(dir / "file.vcf.gz");
        writer.write(contents);
    }
    test_validate(dir);

    EXPECT_EQ(test_height(dir), 4);
    std::vector<size_t> expected_dims { 4, 2 };
    EXPECT_EQ(test_dimensions(dir), expected_dims);
}

TEST_F(VcfExperimentTest, ExpandedParsing) {
    std::string contents = "##fileformat=VCFv4\n";
    contents += "##aasdasd\n";
    contents += "##foobarbar\n";
    contents += "#CHROM\tPOS\tID\tREF\tALT\tQUAL\tFILTER\tINFO\tFORMAT\tsam1\tsam2\n";
    contents += "chr1\t1\tfoo1\tA\tC\t10\tPASS\tNS=1\tGT\t1|0\t0|0\n";

    {
        std::ofstream handle(dir / "OBJECT");
        handle << "{ \"type\": \"vcf_experiment\", \"vcf_experiment\": { \"version\": \"1.0\", \"dimensions\": [7, 2], \"expanded\": true } }";

        byteme::GzipFileWriter writer(dir / "file.vcf.gz");
        writer.write(contents);
        writer.write("chr1\t2\tfoo2");
    }
    expect_error("premature end");

    {
        byteme::GzipFileWriter writer(dir / "file.vcf.gz");
        writer.write(contents);
        writer.write("chr1\t2\tfoo2\n");
    }
    expect_error("premature end");

    {
        byteme::GzipFileWriter writer(dir / "file.vcf.gz");
        writer.write(contents);
        writer.write("chr1\t2\tfoo2\tA\t");
    }
    expect_error("premature end");

    {
        byteme::GzipFileWriter writer(dir / "file.vcf.gz");
        writer.write(contents);
        writer.write("chr1\t2\tfoo2\tA\tC,T");
    }
    expect_error("premature end");

    {
        byteme::GzipFileWriter writer(dir / "file.vcf.gz");
        writer.write(contents);
        writer.write("chr1\t2\tfoo2\tA\tC,T\n");
    }
    expect_error("premature end");

    {
        byteme::GzipFileWriter writer(dir / "file.vcf.gz");
        writer.write(contents);
        writer.write("chr1\t2\tfoo2\tA\tC,T\t");
    }
    expect_error("premature end");

    contents += "chr1\t2\tfoo2\tA\tC,T\t10\tPASS\tNS=1\tGT\t1|0\t0|0\n";
    contents += "chr1\t3\tfoo3\tA\t.\t10\tPASS\tNS=1\tGT\t1|0\t0|0\n";
    contents += "chr1\t4\tfoo4\tAGGGG\tACTG,<DEL>,<MUL>\t10\tPASS\tNS=1\tGT\t1|0\t0|0";
    {
        byteme::GzipFileWriter writer(dir / "file.vcf.gz");
        writer.write(contents);
    }
    expect_error("premature end");

    contents += "\n";
    {
        byteme::GzipFileWriter writer(dir / "file.vcf.gz");
        writer.write(contents);
    }
    test_validate(dir);

    EXPECT_EQ(test_height(dir), 7);
    std::vector<size_t> expected_dims { 7, 2 };
    EXPECT_EQ(test_dimensions(dir), expected_dims);
}
