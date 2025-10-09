cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(seq_parser_tests)

# Lisää parserin lähdekoodi ja testit
target_sources(app PRIVATE
  ../../src/seq_parser.c
  src/test_seq_parser.c
)

# Lisää include-hakemisto, jotta testit löytävät seq_parser.h
target_include_directories(app PRIVATE ../../src)
