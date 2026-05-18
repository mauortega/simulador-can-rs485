# ESP-IDF environment should build only the baremetal entrypoint.
# Arduino sources are compiled by the Arduino environment separately.
idf_component_register(SRCS "main_baremetal.c")
