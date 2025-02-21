include(CPM)
CPMAddPackage("gh:ETLCPP/etl#20.38.16")
CPMAddPackage("gh:catchorg/Catch2@3.5.4")
list(APPEND CMAKE_MODULE_PATH ${Catch2_SOURCE_DIR}/extras)

include(CTest)
include(Catch)

add_executable(${PROJECT_NAME}-test)
target_sources(${PROJECT_NAME}-test
    PRIVATE
        endianness.cpp
        float.cpp
        memmapregs.cpp
        variable_bits.cpp
)
target_link_libraries(${PROJECT_NAME}-test
    PRIVATE
        Catch2::Catch2WithMain
        etl
        ${PROJECT_NAME}
)
catch_discover_tests(${PROJECT_NAME}-test)
