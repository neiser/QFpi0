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



# really no optimization in debug mode
if(CMAKE_COMPILER_IS_GNUCXX)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -trigraphs")
else()
  message(FATAL_ERROR "Non-gcc compiler not supported")
endif()


add_definitions(-DOSF1 -Df2cFortran -DLINUX -DLIBC6 -DWARPFILL)


include_directories(include)

add_library(ana ${FILES_UTIL} ${FILES_ANA})

