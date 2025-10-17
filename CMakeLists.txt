cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(Viikkotehtava5kolmenpisteenarvoinen)

target_sources(app PRIVATE src/main.c)
target_sources(app PRIVATE src/seq_parser.c)
target_sources(app PRIVATE
  src/main.c
  src/seq_parser.c
  src/TimeParser.c
)
target_include_directories(app PRIVATE src)