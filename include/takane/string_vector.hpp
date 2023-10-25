#ifndef TAKANE_STRING_VECTOR_HPP
#define TAKANE_STRING_VECTOR_HPP

namespace takane {

namespace string_vector {

/**
 * @brief Options for parsing the string vector file.
 */
struct Options {
    /**
     * Whether to load and parse the file in parallel, see `comservatory::ReadOptions` for details.
     */
    bool parallel = false;

    /**
     * Version of the `string_vector` format.
     */
    int version = 1;
};

/**
 * @cond
 */
struct NamesField : public comservatory::DummyStringField {
    void add_missing() {
        throw std::runtime_error("missing values should not be present in the names column");
    }
};
/**
 * @endcond
 */

void validate(const char* path, Options options = Options()) {

}


}

}

#endif
