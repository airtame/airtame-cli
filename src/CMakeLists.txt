set(AIRTAME_CLI_TNAME airtame-cli)

add_executable(${AIRTAME_CLI_TNAME}
        main.c
        wingetopt.c
        threading.c
        ssdp.c
        ssdp_uuid.c
        channel.c
        )

if(APPLE)
    set(AIRTAME_CLI_LIBS jsonrpcc)
elseif(UNIX)
    set(TRAY_LIBS jsonrpcc pthread uuid)
endif()

target_link_libraries(${AIRTAME_CLI_TNAME}
        ${AIRTAME_CLI_LIBS})

add_dependencies(${AIRTAME_CLI_TNAME} jsonrpc-c)

install(TARGETS ${AIRTAME_CLI_TNAME} DESTINATION bin)
