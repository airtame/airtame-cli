project (airtame-cli)

add_executable(airtame-cli
        main.c
        wingetopt.c
        threading.c
        ssdp.c
        ssdp_uuid.c
        channel.c
        )

target_link_libraries (airtame-cli
						jsonrpcc)
