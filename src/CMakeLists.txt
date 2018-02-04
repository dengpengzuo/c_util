set(CMAKE_C_FLAGS "-Wall -std=gnu99")
add_definitions(-D_GNU_SOURCES -DUSE_JEMALLOC)

include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

set(SRC_LIST
        ez_max_heap.c
        ez_daemon.c ez_event.c ez_net.c
        ez_hash.c ez_log.c ez_malloc.c ez_util.c
        ez_rbtree.c ez_list.c ez_rwlock.c
)

# static library
ADD_LIBRARY(ez_cutil_static STATIC ${SRC_LIST})
set_target_properties(ez_cutil_static PROPERTIES OUTPUT_NAME "ez_c")
set_target_properties(ez_cutil_static PROPERTIES CLEAN_DIRECT_OUTPUT 1)
set_target_properties(ez_cutil_static PROPERTIES LINKER_LANGUAGE "C")
set_target_properties(ez_cutil_static PROPERTIES ARCHIVE_OUTPUT_DIRECTORY "${LIBRARY_OUTPUT_PATH}")

# -------------------------------
# shared library
# -------------------------------
#ADD_LIBRARY(ez_cutil_shared SHARED ${SRC_LIST})
#set_target_properties(ez_cutil_shared PROPERTIES OUTPUT_NAME "ez_c")
#set_target_properties(ez_cutil_shared PROPERTIES CLEAN_DIRECT_OUTPUT 1)
#set_target_properties(ez_cutil_shared PROPERTIES LINKER_LANGUAGE "C" )
#set_target_properties(ez_cutil_shared PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${LIBRARY_OUTPUT_PATH}")
