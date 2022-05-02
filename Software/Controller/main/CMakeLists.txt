set(srcs
    "${COMPONENT_DIR}/main.c"
    "${COMPONENT_DIR}/flap_nvs.c"
    "${COMPONENT_DIR}/flap_wifi.c"
    "${COMPONENT_DIR}/flap_http_server.c"
    "${COMPONENT_DIR}/flap_mdns.c"
    "${COMPONENT_DIR}/flap_firmware.c"
    "${COMPONENT_DIR}/flap_socket_server.c"
    "${COMPONENT_DIR}/flap_uart.c"
    "${COMPONENT_DIR}/flap_command.c"
    "${COMPONENT_DIR}/flap_api_v1.c"
)

set(inc_dir
    "${COMPONENT_DIR}/include"
)

set(embed_bin
    "${COMPONENT_DIR}/html/index.html"
    "${COMPONENT_DIR}/html/style.css"
    "${COMPONENT_DIR}/html/favicon.svg"
)

idf_component_register(
    SRCS            "${srcs}"
    INCLUDE_DIRS    "${inc_dir}"
    EMBED_FILES     "${embed_bin}"
    # EMBED_TXTFILES  "${embed_txt}"
)