project (airtame-cli)

add_executable(airtame-cli
        main.c
        wingetopt.c
        threading.c
        ssdp.c
        ssdp_uuid.c
        channel.c
        )

# TODO: find a better way of bulding and using the dependencies
include_directories(${CMAKE_SOURCE_DIR}/deps/out/include/)
link_directories(${CMAKE_SOURCE_DIR}/deps/out/lib/)

target_link_libraries (airtame-cli jsonrpcc)
