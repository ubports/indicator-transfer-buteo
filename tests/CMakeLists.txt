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
    ${ACCOUNTS_QT5_LIBRARIES}
)

include_directories(
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${TRANSFER_INDICATOR_INCLUDE_DIRS}
    ${URL_DISPATCHER_INCLUDE_DIRS}
    ${ACCOUNTS_QT5_INCLUDE_DIRS}
)

qt5_use_modules(tst-transfer-plugin Core Test)
add_definitions(-std=c++11)

find_program(DBUS_RUNNER_BIN dbus-test-runner)
add_test(NAME tst-transfer-plugin
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND ${DBUS_RUNNER_BIN}
            --task ${CMAKE_CURRENT_SOURCE_DIR}/buteo-syncfw.py -r -n buteo-syncfw
            --task ${CMAKE_CURRENT_BINARY_DIR}/tst-transfer-plugin --wait-for=com.meego.msyncd -n tst-transfer-plugin
)

# Coverage reporting
include(EnableCoverageReport)
enable_coverage_report(
  TARGETS
    tst-transfer-plugin
  TESTS
    tst-transfer-plugin
)
