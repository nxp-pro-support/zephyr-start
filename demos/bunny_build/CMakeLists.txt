cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(hello_zephyr)

target_sources(app PRIVATE  src/main.c
                            src/t2.c)

target_sources(app PRIVATE ../../../modules/bunny_build/src/bunny_build.c)
target_sources(app PRIVATE ../../../modules/bunny_build/src/ezh_init.c)
target_sources(app PRIVATE src/ezh_app.c)

target_include_directories(app PRIVATE ../../../modules/bunny_build/src)
target_include_directories(app PRIVATE ./src)                            