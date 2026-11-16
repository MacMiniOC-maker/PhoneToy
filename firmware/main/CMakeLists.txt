idf_component_register(
    SRCS "main.c" "audio.c" "io.c"
    INCLUDE_DIRS "."
    REQUIRES driver esp_timer
    EMBED_FILES "ring.pcm")
