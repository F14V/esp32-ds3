idf_component_register(
        SRCS
                "src/ds3.c"
                "src/ds3_bt.c"
                "src/ds3_l2cap.c"
                "src/ds3_parser.c"
        REQUIRES nvs_flash bt
        PRIV_REQUIRES bt
        INCLUDE_DIRS src/include
        PRIV_INCLUDE_DIRS
                ${IDF_PATH}/components/bt/common/include/
                ${IDF_PATH}/components/bt/host/bluedroid/common/include/
                ${IDF_PATH}/components/bt/host/bluedroid/stack/include/
)