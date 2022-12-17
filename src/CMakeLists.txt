add_library(libint
        libint_internal.h
        libint_signed.c
        libint_unsigned.c
        )
target_link_libraries(libint
        PUBLIC libint_interface)
