# Launch genere binary
# cmake-format: off
# The funtion api is almost the same as genere:
#
# Genere <ClassName> <ClassLabel> <AttributeFileName> <LogFile> [options]
#   <ClassName>: base name for generated classes
#   <ClassLabel>: label for generated classes
#   <AttributeFileName>: name of the file (.dd) containing the attribute specifications
#   <LogFile>: name of the log file, it must be included in the source files of the binary/library
#
# Options:
#   -nomodel no generation of class <ClassName>
#   -noarrayview no generation of class <ClassName>ArrayView
#   -noview no generation of classes <ClassName>View and <ClassName>ArrayView
#   -nousersection no generation of user sections
#   -specificmodel <SpecificModelClassName> name of a specific model class, to use instead of ClassName
#   -super <SuperClassName> name of the parent class
#
# Do nothing if GENERATE_VIEWS is set to OFF
#
# Example to use genere binary in CMakeLists.txt:
#
#   include(genere)
#    generate_gui_add_view(FOO "Foo Example" Foo.dd Foo.dd.log -noarrayview)
#   add_executable(basetest Foo.cpp) 
#   if(GENERATE_VIEWS)
#       target_sources(basetest PRIVATE Foo.dd.log) # Foo.dd.log is genereted by  generate_gui_add_view
#   endif()
#    generate_gui_add_view_add_dependency(basetest)
#
# The log file Foo.dd.log is generated by  generate_gui_add_view and must be added as a source of the target. 
# Whithout it, generate_gui_add_view is never triggered.
# 
# Note for developpers: we cannot set the h/cpp view files as output of add_custom_command because they are inputs 
# and outputs. They would be removed at each clean. So we use the logs as output of add_custom_command and as 
# source files of the binary/library target. This way we have the desired mechanic: The views are generated 
# only when GENERATE_VIEWS=ON and a .dd file was modified.
#
# cmake-format: on

function(generate_gui_add_view ClassName ClassLabel AttributeFileName LogFile)

  if(GENERATE_VIEWS)
    find_package(Python REQUIRED)
    add_custom_command(
      OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/${LogFile}
      COMMAND
        ${Python_EXECUTABLE} ${CMAKE_SOURCE_DIR}/scripts/generate_gui.py
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/genere ${ARGN} ${ClassName} ${ClassLabel} ${AttributeFileName} ${LogFile}
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      VERBATIM
      DEPENDS ${AttributeFileName}
      MAIN_DEPENDENCY ${AttributeFileName}
      COMMENT "Generating ${AttributeFileName} views")
  endif()
endfunction()

# Create dependency between the target parameter and the genere binary. This function should be used with
# generate_gui_add_view to ensure that:
#
# - genere is built before the target
# - the target is re-built if genere is modified
#
# Do nothing if GENERATE_VIEWS is set to OFF
function(generate_gui_add_view_add_dependency target)
  if(GENERATE_VIEWS)
    add_dependencies(${target} genere)
  endif()
endfunction()

# Helper function: it builds the list of the .dd.log files starting from the .dd file in the source directory. The list
# is empty if GENERATE_VIEWS is set to OFF.
function(generate_gui_add_view_get_log_list OutVariable)
  if(GENERATE_VIEWS)
    file(
      GLOB ddlogfiles
      RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
      ${CMAKE_CURRENT_SOURCE_DIR}/*.dd)
    list(TRANSFORM ddlogfiles APPEND ".log")
    set(${OutVariable}
        ${ddlogfiles}
        PARENT_SCOPE)
  endif()
endfunction()
