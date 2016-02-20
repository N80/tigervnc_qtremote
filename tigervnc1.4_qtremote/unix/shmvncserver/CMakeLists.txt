include_directories(${CMAKE_SOURCE_DIR}/unix/tx)
include_directories(${CMAKE_SOURCE_DIR}/unix)
include_directories(${CMAKE_SOURCE_DIR}/common)

add_executable(shmvncserver
  buildtime.c
  Geometry.cxx
  Image.cxx
  PollingManager.cxx
  PollingScheduler.cxx
  TimeMillis.cxx
  shmvncserver.cxx
  XPixelBuffer.cxx
  View.cxx
)

target_link_libraries(shmvncserver rfb network rdr)

install(TARGETS shmvncserver DESTINATION ${BIN_DIR})
install(FILES shmvncserver.man DESTINATION ${MAN_DIR}/man1 RENAME shmvncserver.1)
