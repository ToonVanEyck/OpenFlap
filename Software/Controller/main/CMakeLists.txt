set(srcs
    "${COMPONENT_DIR}/../../common/flap_utils.c"
    "${COMPONENT_DIR}/main.c"
    "${COMPONENT_DIR}/flap_nvs.c"
    "${COMPONENT_DIR}/flap_wifi.c"
    "${COMPONENT_DIR}/flap_http_server.c"
    "${COMPONENT_DIR}/flap_mdns.c"
    "${COMPONENT_DIR}/flap_firmware.c"
    "${COMPONENT_DIR}/flap_socket_server.c"
    "${COMPONENT_DIR}/flap_uart.c"
    "${COMPONENT_DIR}/HttpApi/HttpApi.c"
    "${COMPONENT_DIR}/UartApi/UartApi.c"
    "${COMPONENT_DIR}/OpenFlapModel/Model.c"
)

set(inc_dir
    "${COMPONENT_DIR}/include"
    "${COMPONENT_DIR}/HttpApi/include"
    "${COMPONENT_DIR}/UartApi/include"
    "${COMPONENT_DIR}/OpenFlapModel/include"
)

set(embed_bin
    "${COMPONENT_DIR}/html/index.html"
    "${COMPONENT_DIR}/html/style.css"
    "${COMPONENT_DIR}/html/script.js"
    "${COMPONENT_DIR}/html/favicon.svg"
)

idf_component_register(
    SRCS            "${srcs}"
    INCLUDE_DIRS    "${inc_dir}"
    EMBED_FILES     "${embed_bin}"
    # EMBED_TXTFILES  "${embed_txt}"
)