project(mock-textra)

set(SOURCE_ROOT ../../../third_party/lynx-textra/src/textlayout)

file(GLOB TEXTRA_SOURCE
        ${SOURCE_ROOT}/*.cc
        ${SOURCE_ROOT}/style/*.cc
        ${SOURCE_ROOT}/run/*.cc
        ${SOURCE_ROOT}/internal/*.cc
        ${SOURCE_ROOT}/utils/*.cc
)

list(FILTER TEXTRA_SOURCE EXCLUDE REGEX "${SOURCE_ROOT}/utils/icu_wrapper.*")

add_library(mock_textra STATIC ${TEXTRA_SOURCE})
