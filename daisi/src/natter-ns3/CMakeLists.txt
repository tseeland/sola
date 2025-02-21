# Simulator version of natter
add_library(natter_minhcast_sim STATIC)
target_sources(natter_minhcast_sim
        PRIVATE
        $<TARGET_OBJECTS:natter_minhcast_obj>
)
target_include_directories(natter_minhcast_sim
        PUBLIC
            ${natter_SOURCE_DIR}/include # For public headers
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
        )
target_link_libraries(natter_minhcast_sim
        PRIVATE
            natter_network_facade_sim
            natter_utils_sim
            natter_logging
        PUBLIC
            natter_interface
)

# Utils
add_library(natter_utils_sim STATIC)
target_sources(natter_utils_sim
  PRIVATE
  $<TARGET_OBJECTS:natter_utils_obj>
  ${natter_SOURCE_DIR}/include/natter/uuid.h
  uuid_generator_sim.cpp
)
target_link_libraries(natter_utils_sim
        PUBLIC
            natter_utils_obj
        PRIVATE
            daisi_random_engine
)
target_include_directories(natter_utils_sim
        PRIVATE
            ${natter_SOURCE_DIR}/src/utils
)

# Network facade
add_library(natter_network_facade_sim STATIC)
target_sources(natter_network_facade_sim
  PRIVATE
    ${natter_SOURCE_DIR}/src/core/network_facade.h
)
set_target_properties(natter_network_facade_sim PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(natter_network_facade_sim
  PUBLIC
    solanet_serializer
    NetworkUDPSim
)

# Simulation classes
add_library(daisi_natter_application STATIC)
target_sources(daisi_natter_application
        PRIVATE
        natter_application.cpp
        natter_application.h
        )
target_link_libraries(daisi_natter_application
        PUBLIC
        natter_interface
        daisi_logger_manager
        ns3::libcore
        daisi_natter_logger_ns3
        PRIVATE
        daisi_utils
        natter_logging # Private natter target
)

add_library(daisi_natter_logger_ns3 STATIC)
target_sources(daisi_natter_logger_ns3
        PRIVATE
        natter_logger_ns3.h
        natter_logger_ns3.cpp
        )
target_link_libraries(daisi_natter_logger_ns3
        PUBLIC
            natter_minhcast_sim
            daisi_logging_definitions
        PRIVATE
            ns3::libcore
)

add_library(daisi_natter_node_ns3 STATIC)
target_sources(daisi_natter_node_ns3
        PRIVATE
        natter_node_ns3.cpp
        natter_node_ns3.h
        )
target_link_libraries(daisi_natter_node_ns3
        PUBLIC
            natter_minhcast_sim
            ns3::libcore
            daisi_natter_logger_ns3
)


add_library(daisi_natter_manager STATIC)
target_sources(daisi_natter_manager
        PRIVATE
        natter_manager.cpp
        natter_manager.h
)
target_link_libraries(daisi_natter_manager
        PUBLIC
            daisi_manager
            daisi_natter_application
            daisi_natter_node_ns3
            daisi_natter_static_network_calculation
        PRIVATE
            daisi_random_engine
)
target_include_directories(daisi_natter_manager
        PUBLIC
        ${PROJECT_SOURCE_DIR}/src
)

add_library(daisi_natter_static_network_calculation STATIC)
target_sources(daisi_natter_static_network_calculation
        PRIVATE
        static_network_calculation.cpp
        static_network_calculation.h
        )
target_include_directories(daisi_natter_static_network_calculation
        PUBLIC
        ${PROJECT_SOURCE_DIR}/src
        )
