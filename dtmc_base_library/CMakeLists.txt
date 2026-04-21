# dtmc_base_library/CMakeLists.txt

if(NOT DEFINED CMAKE_SCRIPT_MODE_FILE AND NOT DEFINED CMAKE_FIND_PACKAGE_NAME)
    file(GLOB MY_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/dt*.c)
else()
    file(GLOB MY_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/dt*.c)
endif()

# ---- Zephyr -------------------------------------------------
if (ZEPHYR_BASE OR DEFINED Zephyr_FOUND)

    zephyr_library_named(dtmc_base_library)
    
    zephyr_library_sources(${MY_SOURCES})
    zephyr_library_compile_definitions(_GNU_SOURCE)   # to get M_PI in math.h

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


    set_property(TARGET ${COMPONENT_LIB} APPEND PROPERTY LINK_INTERFACE_MULTIPLICITY 3)

# ---- Linux -------------------------------------------------
else()
    add_library(dtmc_base_library STATIC ${MY_SOURCES})
    target_include_directories(dtmc_base_library PUBLIC ${DTMC_BASE_DIR}/dtmc_base_library/include)
    target_link_libraries(dtmc_base_library PUBLIC dtcore_library)

endif()
