execute_process(COMMAND pth-config --libdir OUTPUT_VARIABLE Pth_LIBRARY_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND pth-config --includedir OUTPUT_VARIABLE Pth_INCLUDE OUTPUT_STRIP_TRAILING_WHITESPACE)

find_library(Pth_LIBS NAMES pth PATHS ${Pth_LIBRARY_DIR} DOC "Path to pth library.")

if (Pth_LIBS AND Pth_INCLUDE)
   message(STATUS "pth-config found ${Pth_INCLUDE} ${Pth_LIBS}")
   set(Pth_FOUND 1)
   set(CMAKE_REQUIRED_LIBRARY ${Pth_LIBS})
   set(CMAKE_REQUIRED_INCLUDES ${Pth_INCLUDE})
else ()
   set(Pth_FOUND 0)
endif () 

# Report the results.
if (NOT Pth_FOUND)
  set(Pth_DIR_MESSAGE "Pth was not found. Make sure Pth_LIBS and Pth_INCLUDE are set.")
  if (Pth_FIND_REQUIRED)
    message(FATAL_ERROR "${Pth_DIR_MESSAGE}")
  elseif (NOT Pth_FIND_QUIETLY)
    message(STATUS "${Pth_DIR_MESSAGE}")
  endif ()
endif ()