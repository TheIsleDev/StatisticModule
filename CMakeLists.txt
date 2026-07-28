set(TARGET Statistic)
project(${TARGET})

add_library(${TARGET} SHARED
    src/Statistic.cpp
)

target_link_libraries(${TARGET} PUBLIC EvrimaCommon)
target_link_libraries(${TARGET} PUBLIC UE4SS)