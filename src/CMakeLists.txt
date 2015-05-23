add_executable(airtame-cli
        main.c
        wingetopt.c
        threading.c
        ssdp.c
        ssdp_uuid.c
        channel.c
        )

if (APPLE)
    set (TRAY_LIBS jsonrpcc)
elseif (UNIX)
    set (TRAY_LIBS jsonrpcc pthread uuid)
endif ()

target_link_libraries (airtame-cli
        "${TRAY_LIBS}")

add_dependencies (airtame-cli jsonrpc-c)

install (TARGETS airtame-cli DESTINATION bin)
