# Applies the folder layout in a groupings file (see cmake/SourceGroups.txt)
# to a list of source files with source_group(), so every IDE generator shows
# the same tree.
#
#   framework_apply_source_groups(<groupings file> <file>...)
#
# Files are absolute paths. Anything the groupings file does not match is
# grouped by its directory under the repository root and listed in one
# configure message so the groupings file can be updated.
function(framework_apply_source_groups groupings_file)
  set(_remaining ${ARGN})
  set(_group "")

  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${groupings_file}")
  file(STRINGS "${groupings_file}" _lines)

  foreach(_line IN LISTS _lines)
    string(STRIP "${_line}" _line)
    if(_line STREQUAL "" OR _line MATCHES "^#")
      continue()
    endif()

    if(_line MATCHES "^\\[(.+)\\]$")
      set(_group "${CMAKE_MATCH_1}")
      continue()
    endif()

    if(_group STREQUAL "")
      message(WARNING "${groupings_file}: '${_line}' appears before any [Group] line")
      continue()
    endif()

    # Each entry is a path or glob; only files still unassigned are taken, so
    # the first matching entry wins.
    file(GLOB _matches LIST_DIRECTORIES false "${CMAKE_SOURCE_DIR}/${_line}")
    set(_taken "")
    foreach(_file IN LISTS _matches)
      list(FIND _remaining "${_file}" _index)
      if(NOT _index EQUAL -1)
        list(APPEND _taken "${_file}")
        list(REMOVE_AT _remaining ${_index})
      endif()
    endforeach()
    if(_taken)
      source_group("${_group}" FILES ${_taken})
    endif()
  endforeach()

  if(_remaining)
    source_group(TREE "${CMAKE_SOURCE_DIR}" FILES ${_remaining})
    set(_names "")
    foreach(_file IN LISTS _remaining)
      file(RELATIVE_PATH _relative "${CMAKE_SOURCE_DIR}" "${_file}")
      list(APPEND _names "${_relative}")
    endforeach()
    list(JOIN _names ", " _names)
    message(STATUS "Not in ${groupings_file}, grouped by folder: ${_names}")
  endif()
endfunction()
