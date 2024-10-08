set(DUMP_SHORTCUTS_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")

set(DUMP_SHORTCUTS_SOURCE
        "${DUMP_SHORTCUTS_SOURCE_DIR}/KeyStrings.h"
        "${DUMP_SHORTCUTS_SOURCE_DIR}/KeyStrings.cpp"
        "${DUMP_SHORTCUTS_SOURCE_DIR}/Main.cpp")
set(DUMP_SHORTCUTS_SOURCE ${DUMP_SHORTCUTS_SOURCE} ${INDEX_OUTPUT_PATH} ${DOC_MANUAL_TARGET_FILES_ABSOLUTE} ${DOC_MANUAL_SHORTCUTS_JS_TARGET_ABSOLUTE} ${DOC_MANUAL_TARGET_IMAGE_FILES_ABSOLUTE})

add_executable(dump-shortcuts ${DUMP_SHORTCUTS_SOURCE})
target_include_directories(dump-shortcuts PRIVATE ${DUMP_SHORTCUTS_SOURCE_DIR})
target_link_libraries(dump-shortcuts PRIVATE common)
set_target_properties(dump-shortcuts PROPERTIES AUTOMOC TRUE)
target_link_options(dump-shortcuts PUBLIC -error-limit=0)

set_compiler_config(dump-shortcuts)
# target_link_options(dump-shortcuts PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../wasm/libc++abi.a)
target_link_options(dump-shortcuts PUBLIC /qtbase/lib/libQt5Core.a)
target_link_options(dump-shortcuts PUBLIC /qtbase/lib/libqtpcre2.a)

# Organize files into IDE folders
source_group(TREE "${COMMON_TEST_SOURCE_DIR}" FILES ${COMMON_TEST_SOURCE})

if(WIN32)
    # Copy DLLs to app directory
    add_custom_command(TARGET dump-shortcuts POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:assimp::assimp>" "$<TARGET_FILE_DIR:dump-shortcuts>"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:freeimage::FreeImage>" "$<TARGET_FILE_DIR:dump-shortcuts>"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:freetype>" "$<TARGET_FILE_DIR:dump-shortcuts>"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:tinyxml2::tinyxml2>" "$<TARGET_FILE_DIR:dump-shortcuts>"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:miniz::miniz>" "$<TARGET_FILE_DIR:dump-shortcuts>"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:fmt::fmt>" "$<TARGET_FILE_DIR:dump-shortcuts>"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:GLEW::GLEW>" "$<TARGET_FILE_DIR:dump-shortcuts>"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:Qt5::Widgets>" "$<TARGET_FILE_DIR:dump-shortcuts>"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:Qt5::Gui>" "$<TARGET_FILE_DIR:dump-shortcuts>"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:Qt5::Core>" "$<TARGET_FILE_DIR:dump-shortcuts>"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:Qt5::Svg>" "$<TARGET_FILE_DIR:dump-shortcuts>"
        COMMAND ${CMAKE_COMMAND} -E make_directory    "$<TARGET_FILE_DIR:dump-shortcuts>/platforms"
        COMMAND ${CMAKE_COMMAND} -E make_directory    "$<TARGET_FILE_DIR:dump-shortcuts>/styles"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:Qt5::QWindowsIntegrationPlugin>" "$<TARGET_FILE_DIR:dump-shortcuts>/platforms"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:Qt5::QWindowsVistaStylePlugin>" "$<TARGET_FILE_DIR:dump-shortcuts>/styles")
endif()
