file(GLOB cppfiles ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
add_library(UPDataPreparation STATIC "${cppfiles}")
set_khiops_options(UPDataPreparation)

target_include_directories(UPDataPreparation PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(UPDataPreparation PUBLIC KDDomainKnowledge)
