add_library(daisi_sola_sola_ns3_wrapper STATIC)
target_sources(daisi_sola_sola_ns3_wrapper
        PRIVATE
        sola_ns3_wrapper.cpp
        sola_ns3_wrapper.h
)
target_link_libraries(daisi_sola_sola_ns3_wrapper
        PUBLIC
        SOLA
        daisi_logger_manager
        ManagementOverlayMINHTONSim
        EventDisseminationMinhcastSim
        daisi_cpps_common_cpps_logger_ns3
)

add_library(daisi_sola_application STATIC)
target_sources(daisi_sola_application
        PRIVATE
        sola_application.h
        sola_application.cpp
)
target_link_libraries(daisi_sola_application
        PUBLIC
        ns3::libcore
        daisi_sola_logger_ns3
        daisi_sola_sola_ns3_wrapper
        PRIVATE
        daisi_utils
        minhton_utils_config_reader
)

add_library(daisi_sola_logger_ns3 STATIC)
target_sources(daisi_sola_logger_ns3
        PRIVATE
        sola_logger_ns3.h
        sola_logger_ns3.cpp
)
target_link_libraries(daisi_sola_logger_ns3
        PUBLIC
        daisi_logging_sqlite_helper
        daisi_cpps_common_cpps_logger_ns3
        PRIVATE
        ns3::libcore
        solanet_serializer
)

add_library(daisi_sola_manager STATIC)
target_sources(daisi_sola_manager
        PRIVATE
        sola_manager.cpp
        sola_manager.h
        sola_manager_scheduler.cpp
)
target_link_libraries(daisi_sola_manager
        PUBLIC
        daisi_manager
        daisi_sola_application
        daisi_sola_logger_ns3
)
target_include_directories(daisi_sola_manager
        PUBLIC
        ${PROJECT_SOURCE_DIR}/src
)

# Rebuild SOLA for ns-3 simulation

# EventDisseminationMinhcast
add_library(EventDisseminationMinhcastSim STATIC)
target_sources(EventDisseminationMinhcastSim
        PRIVATE
        ${SOLA_SOURCE_DIR}/src/event_dissemination/natter/event_dissemination_minhcast.cpp
        ${SOLA_SOURCE_DIR}/include/SOLA/event_dissemination_minhcast.h event_dissemination_minhcast_sim.cpp
)
target_link_libraries(EventDisseminationMinhcastSim
        PUBLIC
        SOLAMessage
        EventDissemination
        natter_minhcast_sim
        minhton_core_node_sim
        Storage
        PRIVATE
        sola_check
        ns3::libcore
)
target_include_directories(EventDisseminationMinhcastSim PUBLIC ${SOLA_SOURCE_DIR}/include)

# Management Overlay MINHTON
add_library(ManagementOverlayMINHTONSim STATIC $<TARGET_OBJECTS:ManagementOverlayMINHTONObj>)
target_link_libraries(ManagementOverlayMINHTONSim
        PUBLIC
        Storage
        minhton_core_node_sim
        PRIVATE
        minhton_core_definitions
        minhton_utils_config_reader
)
target_include_directories(ManagementOverlayMINHTONSim
        PUBLIC
        ${SOLA_SOURCE_DIR}/include)
