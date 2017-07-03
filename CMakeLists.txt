cmake_minimum_required(VERSION 2.8.4)

option(test "Build all tests." OFF) # Makes boolean 'test' available.

set(PROJECT_NAME nicolas)
project(${PROJECT_NAME})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -std=c++11")

include_directories(${CMAKE_SOURCE_DIR}/src)

# boost
IF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    # OS X
    set(FASTBIF_LIBRARY "/usr/local/lib/libfastbit.0.dylib")
    set(CMAKE_CXX_FLAGS  "-std=c++11 -stdlib=libc++")

    find_package(Boost COMPONENTS filesystem program_options system random REQUIRED)
    IF (Boost_FOUND)
        INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIR})
        ADD_DEFINITIONS( "-DHAS_BOOST" )
    ENDIF()
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

IF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    # Linux
    set(FASTBIF_LIBRARY "~/usr/local/lib/libfastbit.so")
    include_directories("~/usr/local/include")
    set(CMAKE_CXX_FLAGS  "-std=c++11")

    set(Boost_LIBRARIES "~/usr/local/lib/libboost_system.so;~/usr/local/lib/libboost_filesystem.so;~/usr/local/lib/libboost_program_options.so")

    ADD_DEFINITIONS( "-DLINUX" )

    # openmp
    find_package(OpenMP)
    if (OPENMP_FOUND)
        set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
        set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    endif()
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

# protobuf
#include(FindProtobuf)
#find_package(Protobuf REQUIRED)
#include_directories(${PROTOBUF_INCLUDE_DIR})
#include_directories(${PROJECT_SOURCE_DIR}/proto)
#PROTOBUF_GENERATE_CPP(ProtoSources ProtoHeaders src/nicolas/proto/queries.proto)

# pthread
# find_package (Threads)

# fastbit
set(FASTBIT_FILES src/fastbit/bitvector.cpp src/fastbit/bitvector.h
                  src/fastbit/fileManager.cpp src/fastbit/fileManager.h
                  src/fastbit/resource.cpp src/fastbit/resource.h
                  src/fastbit/util.cpp src/fastbit/util.h
                  src/fastbit/array_t.cpp src/fastbit/array_t.h)

# nicolas
set(UB_SOURCE_FILES src/nicolas/nicolas.cpp src/nicolas/table.cpp src/nicolas/tablex.cpp src/nicolas/table.h src/nicolas/util.h src/nicolas/util.cpp)
set(UCB_SOURCE_FILES src/ucb/table.h src/ucb/table.cpp)
set(NAIVE_SOURCE_FILES src/naive/table.h src/naive/table.cpp)
add_executable(nicolas ${UB_SOURCE_FILES} ${UCB_SOURCE_FILES} ${NAIVE_SOURCE_FILES} ${FASTBIT_FILES} src/nicolas/perf.h)
target_link_libraries(nicolas ${Boost_LIBRARIES})

# bench
# add_executable(bench src/nicolas/bench.cpp src/nicolas/util.h src/nicolas/util.cpp ${ProtoSources} ${ProtoHeaders})
# target_link_libraries(bench ${Boost_LIBRARIES} ${FASTBIF_LIBRARY} ${PROTOBUF_LIBRARY})

# ucb
# add_executable(ucb src/ucb/ucb.cpp src/ucb/table.h src/ucb/table.cpp ${FASTBIT_FILES} src/nicolas/util.h src/nicolas/util.cpp)
# target_link_libraries(ucb ${Boost_LIBRARIES})

# naive
# add_executable(naive src/naive/naive.cpp src/naive/table.h src/naive/table.cpp ${FASTBIT_FILES} src/nicolas/util.h src/nicolas/util.cpp)
# target_link_libraries(naive ${Boost_LIBRARIES})

# part
# add_executable(part src/part/part.cpp src/part/table.h src/part/table.cpp ${FASTBIT_FILES} src/nicolas/util.h src/nicolas/util.cpp)
# target_link_libraries(part ${Boost_LIBRARIES})

# disk
# add_executable(disk src/disk/disk.cpp src/disk/table.h src/disk/table.cpp ${FASTBIT_FILES} src/nicolas/util.h src/nicolas/util.cpp)
# target_link_libraries(disk ${Boost_LIBRARIES})


################################
# Testing
################################
#if (test)
#  # This adds another subdirectory, which has 'project(gtest)'.
#  add_subdirectory(src/gtest)
#
#  enable_testing()
#
#  # Include the gtest library. gtest_SOURCE_DIR is available due to
#  # 'project(gtest)' above.
#  # include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})
#
#  ##############
#  # Unit Tests
#  ##############
#  add_executable(runUnitTests src/nicolas/test/test_common.cpp)
#
#  # Standard linking to gtest stuff.
#  target_link_libraries(runUnitTests gtest gtest_main)
#
#  # Extra linking for the project.
#  target_link_libraries(runUnitTests ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
#
#  # You can also omit NAME and COMMAND. The second argument could be some other
#  # test executable.
#  add_test(common-test runUnitTests)
#endif()
