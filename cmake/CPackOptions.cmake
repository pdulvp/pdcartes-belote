# Read by cpack as it runs, which is after the build: by then this build has
# stamped itself with the time it was made, and that stamp is what separates
# one package of a version from the next. Setting it in CMakeLists would
# freeze it at the moment the project was configured, which may be weeks of
# builds ago.

if(EXISTS "${CPACK_BUILD_VERSION_FILE}")
    file(READ "${CPACK_BUILD_VERSION_FILE}" _pdcartes_belote_build)
    string(STRIP "${_pdcartes_belote_build}" _pdcartes_belote_build)
    if(_pdcartes_belote_build)
        set(CPACK_DEBIAN_PACKAGE_RELEASE "${_pdcartes_belote_build}")
    endif()
endif()
