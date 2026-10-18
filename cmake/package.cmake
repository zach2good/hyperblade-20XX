# The drop-in folder: the image, its pak and SDL3 side by side, for unpacking into a HyperBlade
# folder beside HYPERX.EXE. cmake --install lays it out and cpack archives it.
install(TARGETS hyperx RUNTIME DESTINATION .)
install(FILES "${HYPERX_OUT}/${HYPERX_PAK_NAME}" DESTINATION .)

if(WIN32)
    install(FILES $<TARGET_RUNTIME_DLLS:hyperx> DESTINATION .)
else()
    set_target_properties(hyperx PROPERTIES INSTALL_RPATH "$ORIGIN")
    install(FILES $<TARGET_FILE:SDL3::SDL3> $<TARGET_SONAME_FILE:SDL3::SDL3> DESTINATION .)
endif()

set(CPACK_PACKAGE_NAME hyperblade_20XX)
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
set(CPACK_PACKAGE_DIRECTORY "${CMAKE_BINARY_DIR}/packages")

# The release names the archives without a version, so the install link stays the same.
if(WIN32)
    set(CPACK_GENERATOR ZIP)
    set(CPACK_PACKAGE_FILE_NAME hyperblade_20XX)
else()
    set(CPACK_GENERATOR TGZ)
    set(CPACK_PACKAGE_FILE_NAME hyperblade_20XX-linux)
endif()

include(CPack)
