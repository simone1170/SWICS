set(target_prefix test_)

# Function to create a scratch executable for each source file
function(create_test_executable source_files)
  # Return early if no sources
  list(LENGTH source_files number_sources)
  if(number_sources EQUAL 0)
    return()
  endif()

  # Find the source with the main function
  set(test_src)
  foreach(source_file ${source_files})
    file(READ ${source_file} source_file_contents)
    string(REGEX MATCHALL "main[(| (]" main_position "${source_file_contents}")
    if(CMAKE_MATCH_0)
      set(test_src ${source_file})
    endif()
  endforeach()

  if(NOT test_src)
    return()
  endif()

  # Get the source name and directory
  get_filename_component(test_name ${test_src} NAME_WE)

  # Create an executable for the test source
  add_executable(${target_prefix}${test_name} "${source_files}")

  # Link against the ns-3 libraries
  target_link_libraries(${target_prefix}${test_name} ${LIB_AS_NEEDED_PRE} "${ns3-libs}" "${ns3-contrib-libs}"
  "${ns3-external-libs}" ${LIB_AS_NEEDED_POST})
endfunction()

# Scan *.cc files in the test directory and build a target for each
file(GLOB_RECURSE test_sources CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.cc)
create_test_executable("${test_sources}")
