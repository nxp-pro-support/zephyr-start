cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(hello_zephyr)

target_sources(app PRIVATE  src/main.c )


target_sources_ifdef(CONFIG_APP_ENABLE_T2    app PRIVATE    src/t2.c)

#if(CONFIG_APP_ENABLE_T2)
    #target_sources(app PRIVATE src/t2.c)
#endif()


                            