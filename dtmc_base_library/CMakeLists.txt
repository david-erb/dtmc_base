# dtmc_base_library/CMakeLists.txt

include(${CMAKE_CURRENT_LIST_DIR}/../dthelpers.cmake)
dthelpers_glob(MY_SOURCES dt*.c)

# ---- Zephyr -------------------------------------------------
if (ZEPHYR_BASE OR DEFINED Zephyr_FOUND)

    zephyr_library_named(dtmc_base_library)
    
    zephyr_library_sources(${MY_SOURCES})

    zephyr_library_include_directories(
        ${DTMC_BASE_DIR}/dtmc_base_library/include
        ${DTCORE_DIR}/dtcore_library/include
    )

    zephyr_library_link_libraries(
        dtcore_library
    )
    
# ---- ESP-IDF -------------------------------------------------
    
elseif(DEFINED ENV{IDF_PATH})
    idf_component_register(
        SRCS ${MY_SOURCES}
        INCLUDE_DIRS include
        REQUIRES dtcore_library
    )

else()
    add_library(dtmc_base_library STATIC ${MY_SOURCES})
    target_include_directories(dtmc_base_library PUBLIC ${DTMC_BASE_DIR}/dtmc_base_library/include)
    target_link_libraries(dtmc_base_library PUBLIC dtcore_library)

endif()
