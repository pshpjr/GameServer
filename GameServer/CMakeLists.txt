cmake_minimum_required(VERSION 3.28)

set(CMAKE_CXX_STANDARD 23)
if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    message(STATUS "MSVC")
    add_compile_options(-translateInclude)
    add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /DEBUG")
endif ()




add_executable(CMakeServer
        Range.cpp
        RangeObject.cpp
        Player.cpp
        GameObject.cpp
        Field.cpp
        Server.cpp
        DBThreadWrapper.cpp
        IVictimSelect.cpp
        DBData.cpp
        Sector.cpp
        Monster.cpp
        main.cpp
        ChatCharacter.cpp
        ChannelManager.cpp
        GameDataLoader.cpp
        TextFileReader.cpp
        flat_unordered_set.cpp
        AttackData.cpp
        AttackData.cpp
        MonsterSpawner.cpp
        MonsterSpawner.h
        MonsterAi.cpp
        MonsterAi.h
        Updatable.h
        MoveComponent.cpp
        MoveComponent.h
        IdTypes.h
        AiComponent.cpp
        AiComponent.h
        ThreadPool.cpp
        ThreadPool.h
        DatabaseManager.cpp
        DatabaseManager.h
        FieldData.h
)

target_include_directories(CMakeServer PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/src/Common)

target_compile_definitions(CMakeServer PRIVATE PSH_SERVER)

target_link_libraries(CMakeServer PRIVATE NetLib)

if (POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif ()

#find_package(Boost REQUIRED NO_MODULE)
#target_include_directories(CMakeServer PRIVATE ${Boost_INCLUDE_DIRS})
# Configuration files are visible in the IDE and deployed beside the executable.
file(GLOB GAME_SERVER_CONFIG_FILES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/*.toml"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.toml")
target_sources(CMakeServer PRIVATE ${GAME_SERVER_CONFIG_FILES})
source_group("Config" FILES ${GAME_SERVER_CONFIG_FILES})
set_source_files_properties(${GAME_SERVER_CONFIG_FILES} PROPERTIES HEADER_FILE_ONLY TRUE)
# Ninja/Make relink when only a configuration file changes, rerunning POST_BUILD.
set_property(TARGET CMakeServer APPEND PROPERTY LINK_DEPENDS ${GAME_SERVER_CONFIG_FILES})
add_custom_command(TARGET CMakeServer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${GAME_SERVER_CONFIG_FILES} "$<TARGET_FILE_DIR:CMakeServer>"
    COMMAND_EXPAND_LISTS
    VERBATIM)
