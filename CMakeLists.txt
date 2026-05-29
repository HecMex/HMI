cmake_minimum_required(VERSION 3.16)

project(sample01 VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Quick SerialBus Sql)

qt_standard_project_setup(REQUIRES 6.10)

qt_add_executable(appsample01
    main.cpp
)

qt_add_qml_module(appsample01
    URI sample01
    QML_FILES
        Main.qml
    SOURCES
        excavatorcontroller.h excavatorcontroller.cpp
        excavatordatasource.h
        udpdatasource.h udpdatasource.cpp
        canbusdatasource.h canbusdatasource.cpp
        configmanager.h configmanager.cpp
    RESOURCES
        sample01.qrc
        QML_FILES TelemetryPanel.qml
        QML_FILES ExcavatorVisualizer.qml
        QML_FILES AlertPanel.qml
        QML_FILES config.json config.prod.json config.test.json
        SOURCES sqlservice.h sqlservice.cpp
        RESOURCES README.md
    # Nota: config.json fue eliminado de aquí para evitar la advertencia de Qt6
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(appsample01 PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.appsample01
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(appsample01
    PRIVATE
        Qt6::Quick
        Qt6::SerialBus
        Qt6::Sql
)

install(TARGETS appsample01
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# --- CORRECCIÓN DE LA COPIA DE CONFIG.JSON ---

set(CONFIG_SOURCE_FILE "${CMAKE_CURRENT_SOURCE_DIR}/config.json")

# En entornos MSVC (Windows), el ejecutable final se guarda en subcarpetas tipo Debug/ o Release/
# Usar $<TARGET_FILE_DIR:appsample01> garantiza que el JSON caiga exactamente al lado del .exe final
set(CONFIG_BINARY_FILE "$<TARGET_FILE_DIR:appsample01>/config.json")

add_custom_command(
    TARGET appsample01 POST_BUILD  # <-- SOLUCIÓN: Cambiado a 'appsample01' para que coincida con el ejecutable
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CONFIG_SOURCE_FILE}
    ${CONFIG_BINARY_FILE}
    COMMENT "Actualizando config.json en el directorio del ejecutable final..."
)
