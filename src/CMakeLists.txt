set(BUTEO_TRANSFERS_PLUGIN buteo-transfers)
set(BUTEO_TRANSFERS_SRCS
    buteo-plugin.cpp
    buteo-plugin.h
    buteo-source.cpp
    buteo-source.h
    buteo-transfer.cpp
    buteo-transfer.h
)

add_library(${BUTEO_TRANSFERS_PLUGIN} MODULE
    ${BUTEO_TRANSFERS_SRCS}
)

include_directories(
    ${CMAKE_BINARY_DIR}
    ${GMODULE_INCLUDE_DIRS}
    ${TRANSFER_INDICATOR_INCLUDE_DIRS}
    ${URL_DISPATCHER_INCLUDE_DIRS}
    ${ACCOUNTS_QT5_INCLUDE_DIRS}
)

target_link_libraries(${BUTEO_TRANSFERS_PLUGIN}
    Qt5::Core
    Qt5::DBus
    Qt5::Xml
    ${GMODULE_LIBRARIES}
    ${TRANSFER_INDICATOR_LIBRARIES}
    ${URL_DISPATCHER_LIBRARIES}
    ${ACCOUNTS_QT5_LIBRARIES}
)

add_definitions(-std=c++11)

execute_process(
    COMMAND pkg-config --variable=plugindir indicator-transfer
    OUTPUT_VARIABLE INTDICATOR_TRANSFER_PLUGINDIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

install(TARGETS ${BUTEO_TRANSFERS_PLUGIN}
    DESTINATION ${INTDICATOR_TRANSFER_PLUGINDIR}/
)
