add_library(mbed-bluepill INTERFACE)

target_sources(mbed-bluepill
    INTERFACE
        PeripheralPins.c
)

target_include_directories(mbed-bluepill
    INTERFACE
        .
)

target_link_libraries(mbed-bluepill INTERFACE mbed-stm32f103x8)