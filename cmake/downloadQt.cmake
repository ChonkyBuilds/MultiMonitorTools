function(download_qt QT_VERSION QT_MODULES TARGET_PLATFORM TARGET_ARCH INSTALL_DIR)
    set(AQT_URL "https://github.com/miurahr/aqtinstall/releases/download/v3.3.0/aqt.exe")
    set(AQT_EXE "${CMAKE_BINARY_DIR}/Qt/aqt.exe")

    # Download aqt.exe if not already available
    if(NOT EXISTS "${AQT_EXE}")
        message(STATUS "Downloading aqt.exe from ${AQT_URL}...")
        file(DOWNLOAD
            "${AQT_URL}"
            "${AQT_EXE}"
            SHOW_PROGRESS
            STATUS DOWNLOAD_STATUS
            LOG DOWNLOAD_LOG
        )
        list(GET DOWNLOAD_STATUS 0 DOWNLOAD_CODE)
        if(NOT DOWNLOAD_CODE EQUAL 0)
            message(FATAL_ERROR "Failed to download aqt.exe: ${DOWNLOAD_LOG}")
        endif()
    endif()

	execute_process(
            COMMAND "${AQT_EXE}" install-qt ${TARGET_PLATFORM} desktop ${QT_VERSION} ${TARGET_ARCH} -m ${QT_MODULES} --outputdir "${INSTALL_DIR}"
            WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
            RESULT_VARIABLE result
        )
        if(NOT result EQUAL 0)
            message(FATAL_ERROR "aqt failed with exit code ${result}")
        endif()

    # Custom target to download Qt
    #execute_process(download_qt ALL
    #    COMMAND "${AQT_EXE}" install-qt ${TARGET_PLATFORM} desktop ${QT_VERSION} ${TARGET_ARCH} -m ${QT_MODULES} --outputdir "${INSTALL_DIR}"
    #    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    #    COMMENT "Downloading Qt ${QT_VERSION} with modules: ${QT_MODULES} for ${TARGET_ARCH}"
    #)
endfunction()
