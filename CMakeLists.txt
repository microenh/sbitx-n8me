cmake_minimum_required(VERSION 3.0.0)
project(sBitx VERSION 0.1.0)

add_compile_definitions(N8ME)

add_subdirectory(ft8_lib)

add_library(sbitxLibs 
  fft_filter.c
  hamlib.c
  i2cbb.c
  ini.c
  macros.c 
  modem_ft8.c
  modems.c 
  queue.c
  remote.c
  sbitx_gtk.c
  sbitx_sound.c
  sbitx_utils.c
  si5351v2.c
  si570.c
	telnet.c
  vfo.c
  # V2 add
  webserver.c
  logbook.c
  mongoose.c
  resampler.c
)

# Add GTK3+
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

# Setup CMake to use GTK+, tell the compiler where to look for headers
# and to the linker where to look for libraries
include_directories(${GTK3_INCLUDE_DIRS})
link_directories(${GTK3_LIBRARY_DIRS})

# Add other flags to the compiler
add_definitions(${GTK3_CFLAGS_OTHER})
target_link_libraries(sbitxLibs ${GTK3_LIBRARIES})
# -----

target_link_libraries(sbitxLibs pthread)

add_executable(sBitx sbitx.c)
target_link_libraries(sBitx
  sbitxLibs
  wiringPi
  asound
  m
  fftw3
  fftw3f
  ncurses
  libft8
  sqlite3
  )

include(CTest)
enable_testing()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)