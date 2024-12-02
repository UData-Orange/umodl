file(GLOB cppfiles ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
add_executable(umodl ${cppfiles} umodl.rc)
set_khiops_options(umodl)
target_link_libraries(umodl PUBLIC MHHistograms KWLearningProblem)
