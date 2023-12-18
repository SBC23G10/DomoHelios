# Be aware this file should not be takes as inspiration on how to set up compilation with the CMake build system when using ESP-IDF, because it directly includes the implementation files.
# This has to be done because the examples are build to test if they are still working and to automatically inform the library if a pull request would break examples.
# To actually include the library in your ESP-IDF project read the documentation especially the Installation section
idf_component_register(
                    SRC_DIRS "./"
                    "include/adc_read" 
                    "include/bt_handle"
                    "include/server"
                    "include/storage"
                    "include/wifi"
                    "include/parse_util"
                    "include/thingsboard"
                    "include/servo_ctrl"
                    "include/solar_sys"
                    "include/json_read_DH_cfg"
                    INCLUDE_DIRS "./" "include"
                    EMBED_FILES "resources/favicon.ico" "resources/upload_script.html"
)
