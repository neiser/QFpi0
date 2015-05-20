set(FILES_UTIL
  util/nid.c
  util/tcpip.c 
  util/t_ansi.c
  util/util.c
  )

set(FILES_ANA
  ana/ana_main.c
  ana/get_buffer.c 
  ana/handle_requests.c
  ana/status.c 
  ana/geometry.c 
  ana/calibration.c
  ana/unpack_event.c 
  ana/fill_standard_spectra.c 
  ana/pack_event.c
  )

set(LIBS
  pawlib
  graflib
  grafX11
  packlib
  kernlib
  mathlib 
#  lapack3
  blas 
  Xt
  m
  X11
  Xext
  Xi
#  g2c
  Xpm
  crypt
  dl
  Xm
  nsl
)

# enable some compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -trigraphs")
set(CMAKE_Fortran_FLAGS "${CMAKE_Fortran_FLAGS} -fno-second-underscore -fno-automatic -ffixed-line-length-none")

# those definitions were copied from the Makefile
add_definitions(-DOSF1 -Df2cFortran -DLINUX -DLIBC6 -DWARPFILL)
include_directories(include)

# make the ana library
add_library(ana ${FILES_UTIL} ${FILES_ANA})

# make admin executable
add_executable(asl_adm adm/admin.c)
target_link_libraries(asl_adm ana)

# control needs some kuipc-generated files
# find_program(KUIPC_EXE kuipc)
# foreach(cdf control/control_def.cdf control/utility_def.cdf)
#   add_custom_command(OUTPUT ${cdf}.c
#     COMMAND ${KUIPC_EXE} -c ${cdf} ${cdf}.c
#     DEPENDS ${cdf})
#   list(APPEND FILES_CDF_C ${cdf}.c)
# endforeach()
# add_executable(asl_control
#   control/utility_for.f
#   control/control.c
#   control/control_exec.c 
# 	control/utility_exec.c
#   ${CDF_C_FILES}
#   )
# SET_TARGET_PROPERTIES(asl_control PROPERTIES LINKER_LANGUAGE C)
# target_link_libraries(asl_control ana ${LIBS})