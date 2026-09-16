find_package(Python3 3.9 REQUIRED COMPONENTS Interpreter)

# Each executable gets its own header; NetLib remains independent of its caller.
function(target_build_version target)
    cmake_parse_arguments(VERSION "" "" "SUBMODULES" ${ARGN})
    if(VERSION_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unknown target_build_version arguments: ${VERSION_UNPARSED_ARGUMENTS}")
    endif()

    set(output_dir "${CMAKE_CURRENT_BINARY_DIR}/generated/${target}/$<CONFIG>")
    set(module_arguments)
    foreach(module IN LISTS VERSION_SUBMODULES)
        list(APPEND module_arguments --submodule "${module}")
    endforeach()

    # Always inspect Git; the script preserves the header's timestamp if unchanged.
    add_custom_target(${target}_build_version
        COMMAND Python3::Interpreter
            "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../scripts/generate_build_version.py"
            --repo "${CMAKE_SOURCE_DIR}"
            --output "${output_dir}/BuildVersion.h"
            --target "${target}"
            --configuration "$<CONFIG>"
            ${module_arguments}
        BYPRODUCTS "${output_dir}/BuildVersion.h"
        VERBATIM
    )
    add_dependencies(${target} ${target}_build_version)
    target_include_directories(${target} PRIVATE "${output_dir}")
endfunction()
