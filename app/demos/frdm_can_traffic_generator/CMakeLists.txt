cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(frdm_can_traffic_generator)

target_sources(app PRIVATE src/main.c)
