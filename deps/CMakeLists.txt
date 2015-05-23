include(externalproject)

# add jsonrpc-c
externalproject_add(jsonrpc-c
        SOURCE_DIR ${DEPS_DIR}/jsonrpc-c
        CONFIGURE_COMMAND pwd &&  autoreconf -vfi && ./configure --prefix=${DEPS_OUT_DIR}
        BUILD_IN_SOURCE 1
        BUILD_COMMAND make install
)

# install all built dependencies
install(DIRECTORY "${DEPS_OUT_DIR}/lib/" DESTINATION lib)
install(DIRECTORY "${DEPS_OUT_DIR}/include/" DESTINATION include)
