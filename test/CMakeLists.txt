include(CPM)
CPMAddPackage("gh:catchorg/Catch2@3.5.4")
list(APPEND CMAKE_MODULE_PATH ${Catch2_SOURCE_DIR}/extras)

include(CTest)
include(Catch)

add_executable(${PROJECT_NAME}-test)
target_sources(${PROJECT_NAME}-test
    PRIVATE
        float.cpp
        memmapregs.cpp
        variable_bits.cpp
        size.test.cpp
        integer.test.cpp
)
target_link_libraries(${PROJECT_NAME}-test
    PRIVATE
        ${PROJECT_NAME}
        Catch2::Catch2WithMain
)
catch_discover_tests(${PROJECT_NAME}-test)
