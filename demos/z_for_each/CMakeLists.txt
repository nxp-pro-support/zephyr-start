cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(z_for_each)

target_sources(app PRIVATE  src/main.c
                            src/t2.c)


                            