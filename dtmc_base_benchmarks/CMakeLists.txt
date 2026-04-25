if(NOT DEFINED CMAKE_SCRIPT_MODE_FILE AND NOT DEFINED CMAKE_FIND_PACKAGE_NAME)
    file(GLOB MY_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/benchmark_*.c)
else()
    file(GLOB MY_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/benchmark_*.c)
endif()

# ---- Zephyr -------------------------------------------------
if (ZEPHYR_BASE OR DEFINED Zephyr_FOUND)
    zephyr_library_named(dtmc_base_benchmarks)
    zephyr_library_sources(${MY_SOURCES})
    target_include_directories(dtmc_base_benchmarks PUBLIC ${DTMC_BASE_DIR}/dtmc_base_benchmarks/include)
    target_link_libraries(dtmc_base_benchmarks PUBLIC dtmc_base_library)
    
# ---- ESP-IDF -------------------------------------------------
elseif(DEFINED ENV{IDF_PATH})
    idf_component_register(
        SRCS ${MY_SOURCES}
        INCLUDE_DIRS include
        REQUIRES 
            dtmc_base_library 
            dtcore_library
    )

# ---- Pico SDK -------------------------------------------------
elseif(DEFINED PICO_BOARD)
    # Build a real static library for the benchmarks
    add_library(dtmc_base_benchmarks STATIC
        ${MY_SOURCES}
    )

    # Publish headers to consumers and also expose dependency headers
    target_include_directories(dtmc_base_benchmarks
        PUBLIC
            ${DTMC_BASE_DIR}/dtmc_base_benchmarks/include
            ${DTMC_BASE_DIR}/dtmc_base_library/include
            ${DTCORE_DIR}/dtcore_library/include
    )

    # Depend on dtcore_library so headers + link dependencies propagate
    target_link_libraries(dtmc_base_benchmarks
        PUBLIC
            dtmc_base_library
            dtcore_library
    )

# ---- else (presumes Linux) -------------------------------------------------
else()
    add_library(dtmc_base_benchmarks STATIC ${MY_SOURCES})
    target_include_directories(dtmc_base_benchmarks PUBLIC ${DTMC_BASE_DIR}/dtmc_base_benchmarks/include)
    target_link_libraries(dtmc_base_benchmarks PUBLIC dtmc_base_library)

endif()
