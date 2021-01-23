include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

# build exe
add_executable(echo_svr echo_s.c)
target_link_libraries(echo_svr jemalloc pthread ez_cutil_static)
set_target_properties(echo_svr PROPERTIES LINKER_LANGUAGE "C" )
set_target_properties(echo_svr PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${EXECUTABLE_OUTPUT_PATH}")

add_executable(echo_cli echo_c.c)
target_link_libraries(echo_cli jemalloc ez_cutil_static)
set_target_properties(echo_cli PROPERTIES LINKER_LANGUAGE "C" )
set_target_properties(echo_cli PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${EXECUTABLE_OUTPUT_PATH}")

add_executable(rbtree_test rbtree_test.c)
target_link_libraries(rbtree_test jemalloc ez_cutil_static)
set_target_properties(rbtree_test PROPERTIES LINKER_LANGUAGE "C" )
set_target_properties(rbtree_test PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${EXECUTABLE_OUTPUT_PATH}")
