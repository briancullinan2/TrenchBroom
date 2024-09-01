# Target operating system and architecture
set(APPLE 0)
set(WIN32 0)
set(WASM 1)
set( CMAKE_SYSTEM_NAME WASI )
set( CMAKE_SYSTEM_PROCESSOR wasm )

# C/C++ compilers
set( CMAKE_C_COMPILER ${CMAKE_SOURCE_DIR}/darwin/wasi-sdk-22.0/bin/clang )
set( CMAKE_CXX_COMPILER ${CMAKE_SOURCE_DIR}/darwin/wasi-sdk-22.0/bin/clang++ )
#set( CMAKE_RC_COMPILER rc )

# Target prefix
set( CMAKE_FIND_ROOT_PATH ${CMAKE_SOURCE_DIR}/darwin/wasi-sdk-22.0/bin )

# Find programs using host paths and headers/libraries using target paths
set( CMAKE_FIND_ROOT_PATH_MODE_PROGRAM ONLY )
set( CMAKE_FIND_ROOT_PATH_MODE_LIBRARY NEVER )
set( CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY )

add_compile_options(--target=wasm32-wasi)
add_compile_options(-D__EMSCRIPTEN__=1)
add_compile_options(-D__WASM__=1)
add_compile_options(-D__wasi__=1)
add_compile_options(-D__wasm32__=1)
add_compile_options(-ftemplate-backtrace-limit=0)
# add_compile_options(-Wno-shadow-uncaptured-local)
# -Wshadow-all in Utils 

set(CMAKE_HAVE_LIBC_PTHREAD FALSE)
set(THREADS_PREFER_PTHREAD_FLAG FALSE)
set(CMAKE_THREAD_LIBS_INIT FALSE)
set(CMAKE_USE_PTHREADS_INIT FALSE)

list(APPEND CMAKE_C_FLAGS -stdlib=gnu11)
list(APPEND CMAKE_CXX_FLAGS -stdlib=libc++)


