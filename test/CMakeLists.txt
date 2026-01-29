include(get_cpm)
CPMAddPackage("gh:boost-ext/ut@2.3.1")
include(CTest)

add_executable(${PROJECT_NAME}-test main.cpp)
target_sources(${PROJECT_NAME}-test
    PRIVATE
        integer.test.cpp
        size.test.cpp
        variable_bits.test.cpp
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:mmreg.test.cpp>
)
target_link_libraries(${PROJECT_NAME}-test
    PRIVATE
        ${PROJECT_NAME}
        ut
)
add_test(NAME ${PROJECT_NAME}-test COMMAND ${PROJECT_NAME}-test)

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    return()
endif()

# https://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html
# https://embeddedartistry.com/blog/2023/09/20/leveraging-your-toolchain-to-improve-security/
target_link_options(${PROJECT_NAME}-test
    PRIVATE
        -fsanitize=bounds,signed-integer-overflow,address,undefined
)
target_compile_options(${PROJECT_NAME}-test
    PRIVATE
        -ftrivial-auto-var-init=pattern
        -fstack-protector-all
        -fno-omit-frame-pointer
        -fsanitize=bounds,signed-integer-overflow,address,undefined
        -Wall
        -Wextra
        -Werror
        -Wformat=2
        -Wsign-conversion
        -Wuninitialized
        -Wcast-qual
        -Wshadow
        -Wimplicit-fallthrough
        -Wfloat-equal
        -Wstack-protector
        $<$<CXX_COMPILER_ID:GNU>:
            -fconcepts-diagnostics-depth=20
            #-fanalyzer
            -Wshift-overflow=2
            -Warray-bounds=2
            -Wstringop-overflow=4
            -Wuse-after-free=3
            -Wformat-overflow=2
            -Wformat-truncation=2
            -Wformat-signedness
            -Wlogical-op
            -Wduplicated-cond
            -Wduplicated-branches
            #-Wswitch-default
            -Wcast-align=strict
        >
        $<$<CXX_COMPILER_ID:Clang>:
            -Wshift-sign-overflow
            -Warray-bounds-pointer-arithmetic
            -Wformat-type-confusion
            -Wcast-align
            -Wconditional-uninitialized
            -Wloop-analysis
            -Wtautological-constant-in-range-compare
            -Wcomma
            -Wassign-enum
            -Wunreachable-code-aggressive
        >
        -Wno-unknown-pragmas
)

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    find_program(CLANG_TIDY_EXE NAMES "clang-tidy" REQUIRED)
    set_target_properties(${PROJECT_NAME}-test
        PROPERTIES
            CXX_CLANG_TIDY "${CLANG_TIDY_EXE}"
    )
    message(STATUS "Enabled clang-tidy for ${PROJECT_NAME}-test")
endif()
