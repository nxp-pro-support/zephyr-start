# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(t1s_device)

target_sources(app PRIVATE src/main.c)

# Iterable section for the HTTP resources of our service (see main.c).
zephyr_linker_sources(SECTIONS sections-rom.ld)
zephyr_linker_section(NAME http_resource_desc_t1s_http_service
  KVMA RAM_REGION GROUP RODATA_REGION
)

# Embed the dashboard, gzipped, into flash.
set(gen_dir ${ZEPHYR_BINARY_DIR}/include/generated/)
generate_inc_file_for_target(app web/index.html ${gen_dir}/index.html.gz.inc --gzip)
