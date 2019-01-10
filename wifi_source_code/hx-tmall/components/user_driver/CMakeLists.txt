# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly

set(COMPONENT_SRCDIRS ".")
set(COMPONENT_ADD_INCLUDEDIRS "include")
set(COMPONENT_REQUIRES  "spi_flash" "nvs_flash" "mbedtls" "wpa_supplicant" "json")
register_component()
