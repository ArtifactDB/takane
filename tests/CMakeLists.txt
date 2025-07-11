include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/7da55820cc32dedd6c1b048f2d4e13fdde5e8237.zip
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Avoid installing GoogleTest when installing this project.
option(INSTALL_GTEST "Enable installation of googletest." OFF)

FetchContent_MakeAvailable(googletest)

enable_testing()

# Main test executable.
add_executable(
    libtest 
    src/data_frame.cpp
    #    src/csv_data_frame.cpp
    src/genomic_ranges.cpp
    src/sequence_information.cpp
    src/atomic_vector.cpp
    src/string_factor.cpp
    src/simple_list.cpp
    src/data_frame_factor.cpp
    src/atomic_vector_list.cpp
    src/data_frame_list.cpp
    src/genomic_ranges_list.cpp
    #    src/factor.cpp
    #    src/compressed_list.cpp
    src/compressed_sparse_matrix.cpp
    src/dense_array.cpp
    src/summarized_experiment.cpp
    src/ranged_summarized_experiment.cpp
    src/single_cell_experiment.cpp
    src/spatial_experiment.cpp
    src/multi_sample_dataset.cpp
    src/sequence_string_set.cpp
    src/bam_file.cpp
    src/bcf_file.cpp
    src/bigwig_file.cpp
    src/bigbed_file.cpp
    src/fasta_file.cpp
    src/fastq_file.cpp
    src/bed_file.cpp
    src/gmt_file.cpp
    src/gff_file.cpp
    src/rds_file.cpp
    src/bumpy_atomic_array.cpp
    src/bumpy_data_frame_array.cpp
    src/vcf_experiment.cpp
    src/delayed_array.cpp
    src/utils_compressed_list.cpp
    src/utils_bumpy_array.cpp
    src/utils_string.cpp
    src/utils_factor.cpp
    src/utils_other.cpp
    src/utils_array.cpp
    src/utils_summarized_experiment.cpp
    src/utils_public.cpp
    src/utils_json.cpp
    src/utils_files.cpp
    src/utils.cpp
    src/dispatch.cpp
)

target_link_libraries(
    libtest
    gtest_main
    gmock_main 
    takane
)

target_compile_options(libtest PRIVATE -Wall -Wextra -Wpedantic -Werror)

set(CODE_COVERAGE OFF CACHE BOOL "Enable coverage testing")
if(CODE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(libtest PRIVATE -O0 -g --coverage)
    target_link_options(libtest PRIVATE --coverage)
endif()

include(GoogleTest)
gtest_discover_tests(libtest DISCOVERY_MODE PRE_TEST)
