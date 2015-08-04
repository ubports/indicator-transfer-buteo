add_executable(tst-transfer-plugin
    tst-transfer-plugin.cpp
    ${CMAKE_SOURCE_DIR}/src/buteo-source.cpp
    ${CMAKE_SOURCE_DIR}/src/buteo-transfer.cpp
)

target_link_libraries(tst-transfer-plugin
    Qt5::Core
    Qt5::DBus
    Qt5::Xml
    ${GMODULE_LIBRARIES}
    ${TRANSFER_INDICATOR_LIBRARIES}
    ${URL_DISPATCHER_LIBRARIES}
)

include_directories(
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${TRANSFER_INDICATOR_INCLUDE_DIRS}
    ${URL_DISPATCHER_INCLUDE_DIRS}
)

qt5_use_modules(tst-transfer-plugin Core Test)
add_test(tst-transfer-plugin tst-transfer-plugin)
add_definitions(-std=c++11)
