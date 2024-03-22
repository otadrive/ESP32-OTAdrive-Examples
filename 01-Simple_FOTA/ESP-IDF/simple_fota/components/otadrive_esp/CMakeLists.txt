set(priv_requires "log" "freertos" "esp_http_client" "esp-tls" "esp_https_ota" "app_update" "esp_timer")
set(requires "esp_event")
set(srcs "src/otadrive_esp.c" "src/conf.c" "src/http.c" "src/misc.c")

idf_component_register(SRCS "${srcs}"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES ${priv_requires}
                    REQUIRES ${requires})
