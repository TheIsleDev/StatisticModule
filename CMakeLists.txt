set(TARGET Statistic)
project(${TARGET})

add_library(${TARGET} SHARED
    src/Statistic.cpp
    src/Subsystem.cpp
    src/Callbacks.cpp
)

target_include_directories(${TARGET} PRIVATE "include")
target_link_libraries(${TARGET} PUBLIC UE4SS)