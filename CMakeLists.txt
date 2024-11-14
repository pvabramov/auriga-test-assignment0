cmake_minimum_required(VERSION 3.10)

project(auriga-test-assignment0
    VERSION "1.0"
    DESCRIPTION "This is a just test assignment project to show UNIX 'ls -l' command implementation"
    LANGUAGES C
)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

add_compile_options(-Wall -Wextra)

set(MYLS_SOURCES
    "ls.c"
    "cache.c"
    "fn_info.c"
)

add_executable(myls ${MYLS_SOURCES})
