#ifndef TAKANE_VCF_EXPERIMENT_HPP
#define TAKANE_VCF_EXPERIMENT_HPP

#include "ranged_summarized_experiment.hpp"
#include "utils_public.hpp"

namespace takane {

/**
 * @cond
 */
void validate(const std::filesystem::path&, const ObjectMetadata&, const Options& options);
size_t height(const std::filesystem::path&, const ObjectMetadata&, const Options& options);
std::vector<size_t> dimensions(const std::filesystem::path&, const ObjectMetadata&, const Options& options);
/**
 * @endcond
 */

namespace vcf_experiment {

namespace internal {

inline void validate_reference_allele(const std::filesystem::path& allele_dir, const Options& options) {
    auto ref_dir = allele_dir / "reference";
    auto ref_meta = read_object_metadata(ref_dir);
    if (ref_meta.type != "sequence_string_set") {
        throw std::runtime_error("'alleles/reference' should contain a 'sequence_string_set' object");
    }

    try {
        ::takane::validate(ref_dir, ref_meta, options);
    } catch (std::exception& e) {
        throw std::runtime_error("failed to validate reference alleles in '" + ref_dir.string() + "'; " + std::string(e.what()));
    }

    const auto& seqtype = internal_json::extract_string_from_typed_object(metadata.other, "sequence_type", "sequence_string_set");
    if (seqtype != "DNA") {
        throw std::runtime_error("'" + ref_dir.string() + "' should contain DNA sequences");
    }

    if (::takane::height(ref_dir, ref_meta, options) != self_dims[0]) {
        throw std::runtime_error("'" + ref_dir.string() + "' should have length equal to the number of rows of the 'vcf_experiment'");
    }
}

inline void validate_alternative_allele(const std::filesystem::path& allele_dir, const internal_json::JsonObjectMap& vcfmap, const Options& options) {
    bool expanded = false;
    auto vIt = vcfmap.find("expanded")
    if (vIt != vcfmap.end()) {
        const auto& val = vIt->second;
        if (val->type() == millijson::BOOLEAN) {
            throw std::runtime_error("expected 'vcf_experiment.expanded' to be a JSON boolean");
        }
        expanded = reinterpret_cast<const millijson::Boolean*>(val.get())->value;
    }

    bool structural = false;
    auto sIt = vcfmap.find("structural")
    if (sIt != vcfmap.end()) {
        const auto& val = vIt->second;
        if (val->type() == millijson::BOOLEAN) {
            throw std::runtime_error("expected 'vcf_experiment.structural' to be a JSON boolean");
        }
        structural = reinterpret_cast<const millijson::Boolean*>(val.get())->value;
    }

    auto alt_dir = allele_dir / "alternative";
    auto alt_meta = read_object_metadata(ref_dir);
    if (expanded) {
        if (structural) {
            if (alt_meta.type != "atomic_vector") {
                throw std::runtime_error("'" + alt_dir.string() + "' should be an atomic vector");
            }
        } else {
            if (alt_meta.type != "sequence_string_set") {
                throw std::runtime_error("'" + alt_dir.string() + "' should be a sequence string set");
            }
        }
    } else {
        if (structural) {
            if (alt_meta.type != "atomic_vector_list") {
                throw std::runtime_error("'" + alt_dir.string() + "' should be an atomic vector list");
            }
        } else {
            // Well, this isn't technically supported yet, but whatever.
            if (alt_meta.type != "sequence_string_set_list") {
                throw std::runtime_error("'" + alt_dir.string() + "' should be a sequence string set list");
            }
        }
    }

    try {
        ::takane::validate(alt_dir, alt_meta, options);
    } catch (std::exception& e) {
        throw std::runtime_error("failed to validate alternative alleles in '" + alt_dir.string() + "'; " + std::string(e.what()));
    }

    if (::takane::height(alt_dir, alt_meta, options) != self_dims[0]) {
        throw std::runtime_error("'" + alt_dir.string() + "' should have length equal to the number of rows of the 'vcf_experiment'");
    }

    if (structural) {
        auto fpath = alt_dir;
        if (!expanded) {
            fpath /= "concatenated";
        }
        fpath /= "contents.h5";

        auto chandle = ritsuko::hdf5::open_file(fpath);
        auto cahandle = ritsuko::hdf5::open_group(chandle, "atomic_vector");
        auto catype = ritsuko::hdf5::open_and_load_scalar_string_attribute(cahandle, "type");
        if (catype != "string") {
            throw std::runtime_error("expected alternative alleles to be stored as strings in '" + alt_dir.string() + "'");
        }

    } else {
        bool is_dna = false;
        if (expanded) {
            const auto& seqtype = internal_json::extract_string_from_typed_object(alt_meta.other, "sequence_type", "sequence_string_set");
            is_dna = (seqtype == "DNA");
        } else {
            const inner_alt_meta = read_object_metadata(alt_dir / "concatenated");
            const auto& seqtype = internal_json::extract_string_from_typed_object(inner_alt_meta.other, "sequence_type", "sequence_string_set");
            is_dna = (seqtype == "DNA");
        }
        if (is_dna) {
            throw std::runtime_error("'" + alt_dir.string() + "' should contain DNA sequences");
        }
    }
}

}

inline void validate(const std::filesystem::path& path, const ObjectMetadata& metadata, const Options& options) {
    ::takane::ranged_summarized_experiment::validate(path, metadata, options);
    auto self_dims = ::takane::dimensions(path, metadata, options);

    const auto& vcfmap = internal_json::extract_typed_object_from_metadata(metadata.other, "vcf_experiment");
    const std::string& vstring = internal_json::extract_string_from_typed_object(vcfmap, "version", "vcf_experiment");
    auto version = ritsuko::parse_version_string(vstring.c_str(), vstring.size(), /* skip_patch = */ true);
    if (version.major != 1) {
        throw std::runtime_error("unsupported version string '" + vstring + "'");
    }

    // Checking that the row ranges only contains a GRanges.
    auto rr_meta = read_object_metadata(path / "row_ranges");
    if (rr_meta.type != "genomic_ranges") {
        throw std::runtime_error("'row_ranges' should contain a 'genomic_ranges' object");
    }

    // Checking the alleles.
    auto allele_dir = path / "alleles";
    internal::validate_reference_allele(allele_dir, options);
    internal::validate_reference_allele(allele_dir, vcfmap, options);


    auto vhandle = ritsuko::hdf5::open_file(path / "variants.h5");
    auto ghandle = ritsuko::hdf5::open_group(vhandle, "vcf_experiment");


    // Checking the fixed metadata.
    {
        auto fhandle = ritsuko::hdf5::open_group(ghandle, "fixed");
    }

}

}

}

#endif
