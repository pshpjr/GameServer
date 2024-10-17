cmake_minimum_required(VERSION 3.28)

set(CMAKE_CXX_STANDARD 20)
if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    message(STATUS "MSVC")
    add_compile_options(-translateInclude)
    add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /DEBUG")
endif ()

#optick 라이브러리 관련 설정.
target_compile_definitions(NetLib PRIVATE USE_OPTICK=1)

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
)

target_include_directories(CMakeServer PRIVATE ../../packetParser)

target_compile_definitions(CMakeServer PRIVATE PSH_SERVER)

target_link_libraries(CMakeServer PRIVATE NetLib)
