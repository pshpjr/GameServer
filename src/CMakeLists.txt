cmake_minimum_required(VERSION 3.28)

set(CMAKE_CXX_STANDARD 20)
if(MSVC)
    add_compile_options(-translateInclude)
    add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
endif()

add_executable(CMakeServer
        Range.cpp
        RangeObject.cpp
        Player.cpp
        GameObject.cpp
        Field.cpp
        Server.cpp
        DBThreadWrapper.h
        Player.h
        Rand.h
        ChatCharacter.h
        GameObject.h
        flat_unordered_set.h
        Server.h
        Monster.h
        Range.h
        DBThreadWrapper.cpp
        TableData.h
        TextFileReader.h
        FieldData.h
        MockData.h
        IVictimSelect.cpp
        DBData.cpp
        Sector.cpp
        Sector.h
        ServerInitData.h
        Monster.cpp
        main.cpp
        IVictimSelect.h
        ChatCharacter.cpp
        MonitorProtocol.h
        GameDataLoader.cpp
        Timer.h
        Field.h
        TextFileReader.cpp
        DBData.h
        AttackData.h
        GameDataLoader.h
        flat_unordered_set.cpp
        GameMap.h
        RangeObject.h
        AttackData.cpp
        AttackData.cpp
        Range.h
        TableData.h
)

target_include_directories(CMakeServer PRIVATE ../../packetParser)

target_compile_definitions(CMakeServer PRIVATE PSH_SERVER)

target_link_libraries(CMakeServer PRIVATE NetLib)
