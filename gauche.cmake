# autotools:
# AC_GAUCHE_INIT_EXT
# AC_GAUCHE_INSTALL_TYPE(sys)
## Get compiler parameters which Gauche has been compiled with.
# AC_GAUCHE_CC
# AC_GAUCHE_FLAGS
## Set LDFLAGS to generate shared library.
# AC_GAUCHE_FIX_LIBS
# AC_GAUCHE_EXT_FIXUP(bdb)

########## Find gauche-config
execute_process(COMMAND gauche-config -I
  OUTPUT_VARIABLE gauche_includes
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
# alt: gauche-config --incdirs
# cut by :

string(REGEX REPLACE "^-I" "" gauche_includes ${gauche_includes})
message("gauche_includes is ${gauche_includes}")


# --archdirs
execute_process(COMMAND gauche-config -L
  OUTPUT_VARIABLE gauche_libs_dir
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
string(REGEX REPLACE "^-L" "" gauche_libs_dir ${gauche_libs_dir})
message("gauche_libs_dir is ${gauche_libs_dir}")

# gauche-config -l
# do I want this?
execute_process(COMMAND gauche-config -l
  OUTPUT_VARIABLE gauche_libs
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
message("gauche_libs is ${gauche_libs} .")

# gauche-config --so-cflags
# -fPIC
# gauche-config --so-ldflags
# -shared -o
execute_process(COMMAND gauche-config --so-cflags
  OUTPUT_VARIABLE gauche_so_cflags
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

# gauche-config --sitearchdir



# thankfully not different
execute_process(COMMAND gauche-config --so-suffix
  OUTPUT_VARIABLE gauche_so_suffix
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(COMMAND gauche-config --sitearchdir
  OUTPUT_VARIABLE gauche_moduledir
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

message("gauche_moduledir is ${gauche_moduledir} .")

# OBJECT_DIR = CMakeFiles/gauche-bdb.dir
# set(gauche_libs "-lgauche-0.98 -lmbedtls -lcrypt -lrt -lm  -lpthread")
#  ;(exec gauche-config -I)


function(gauche_stub root)
  add_custom_command(
    OUTPUT ${root}.c
    # CMAKE_CURRENT_BINARY_DIR
    COMMAND gosh tools/genstub
    # gosh tools/genstub ../bdb.stub
    ${CMAKE_CURRENT_SOURCE_DIR}/${root}.stub
    # ${CMAKE_CURRENT_BINARY_DIR}/${root}.c
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS ${root}.stub
    # CMAKE_CURRENT_SOURCE_DIR
  )
endfunction()
