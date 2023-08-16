cmake_minimum_required(VERSION 3.0.0)
project(libft8 VERSION 0.1.0)

add_library(libft8
  ft8/constants.c
  ft8/encode.c
  ft8/decode.c
  ft8/pack.c
  ft8/text.c
  ft8/crc.c
  ft8/ldpc.c
  ft8/unpack.c
  common/wave.c
  fft/kiss_fftr.c
  fft/kiss_fft.c
)

add_executable(decode_ft8
  decode_ft8.c
) 

target_link_libraries(decode_ft8
  m
  libft8 
)

add_executable(decode_ft82
  decode_ft82.c
)

target_link_libraries(decode_ft82
  m
  libft8
)

add_executable(ft8_shell
  ft8_shell.c
)

target_link_libraries(ft8_shell
  libft8
)

add_executable(gen_ft8
  gen_ft8.c
)

target_link_libraries(gen_ft8
  m
  libft8 
)

add_executable(test_app
  test.c
)

target_link_libraries(test_app
  m
  libft8
)

include(CTest)
enable_testing()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)