cmake_minimum_required(VERSION 3.14)

project(gauche-bdb)

include(gauche.cmake)

# header files take care in build/.ninja_deps
set(sources
  bdb-lib.c
)

# libdb5.3-dev has no cmake, pkg-config .pc
set(BDB_LIBS "-ldb")

# generated
gauche_stub(bdb)

# add_gauche_module()
add_library(${PROJECT_NAME}
  MODULE
  ${sources}
  # stub-processed
  ${CMAKE_CURRENT_SOURCE_DIR}/bdb.c
)

target_include_directories(${PROJECT_NAME}
  SYSTEM PUBLIC
  ${gauche_includes}
  )

target_link_libraries(${PROJECT_NAME} ${BDB_LIBS})
target_link_libraries(${PROJECT_NAME} ${gauche_libs})
# "-g"
target_compile_options(${PROJECT_NAME} PRIVATE ${gauche_so_cflags})

# make it installable?
install(TARGETS ${PROJECT_NAME}
  EXPORT EXPORT_module
  LIBRARY
  # ${CMAKE_INSTALL_PREFIX}  /usr/lib/
  # CMAKE_INSTALL_PREFIX CMAKE_INSTALL_LIBDIR
  DESTINATION "${DESTDIR}${gauche_moduledir}"
  RENAME ${PROJECT_NAME}${gauche_so_suffix}
  )
