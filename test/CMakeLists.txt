add_definitions(-D_GNU_SOURCES)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

# build exe
add_executable(test_s test_s.c)
target_link_libraries(test_s jemalloc pthread ez_cutil_static)
set_target_properties(test_s PROPERTIES LINKER_LANGUAGE "C" )
set_target_properties(test_s PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${EXECUTABLE_OUTPUT_PATH}")

add_executable(test_c test_c.c)
target_link_libraries(test_c jemalloc ez_cutil_static)
set_target_properties(test_c PROPERTIES LINKER_LANGUAGE "C" )
set_target_properties(test_c PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${EXECUTABLE_OUTPUT_PATH}")

add_executable(rbtree_test rbtree_test.c)
target_link_libraries(rbtree_test ez_cutil_static)
set_target_properties(rbtree_test PROPERTIES LINKER_LANGUAGE "C" )
set_target_properties(rbtree_test PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${EXECUTABLE_OUTPUT_PATH}")