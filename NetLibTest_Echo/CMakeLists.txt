add_executable(NetLibTest_Echo
    main.cpp
    Server.cpp
    Server.h
)

target_compile_features(NetLibTest_Echo PRIVATE cxx_std_23)
target_link_libraries(NetLibTest_Echo PRIVATE NetLib)
