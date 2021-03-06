
# If GoogleTest is not installed, build and install a private copy.
# Skip this step if GOOGLE_TEST is not defined.
if(GOOGLE_TEST)
  find_package(GoogleTest REQUIRED)
  if(GTEST_FOUND)
    message(STATUS "Using installed GoogleTest at: ${GTEST_INCLUDE_DIRS}")
  else(GTEST_FOUND)
    message(STATUS "GoogleTest package not found; proceeding to download it.")
    configure_file(GTestExtProject.cmake.in googletest-download/CMakeLists.txt)
    execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
      RESULT_VARIABLE result
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download)
    if(result)
      message(FATAL_ERROR "Failed to download GoogleTest: ${result}.")
    endif()
    message(STATUS "Building GoogleTest.")
    execute_process(COMMAND ${CMAKE_COMMAND} --build .
      RESULT_VARIABLE result
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download)
    if(result)
      message(FATAL_ERROR "Failed to build GoogleTest package: ${result}.")
    endif()
    # Prevent overriding the parent project's compiler/linker
    # settings on Windows
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    # Add googletest directly to our build. This defines
    # the gtest and gtest_main targets.
    add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/googletest-src
                     ${CMAKE_CURRENT_BINARY_DIR}/googletest-build
                     EXCLUDE_FROM_ALL)
    set(GTEST_INCLUDE_DIRS "${gtest_SOURCE_DIR}/include")
    message(STATUS "GoogleTest has been installed succesfully: ${GTEST_INCLUDE_DIRS}.")
  endif(GTEST_FOUND)
endif(GOOGLE_TEST)

# Linking and building unit tests
include_directories(${GTEST_INCLUDE_DIRS}
    ${PERCOLATOR_SOURCE_DIR}/src
    ${PERCOLATOR_SOURCE_DIR}/src/fido
    ${PERCOLATOR_SOURCE_DIR}/data/tests
    ${CMAKE_BINARY_DIR}/src)
add_executable(gtest_unit
    Unit_tests_Percolator_main.cpp
    UnitTest_Percolator_Fido.cpp
    UnitTest_Percolator_Option.cpp)
target_link_libraries(gtest_unit perclibrary fido gtest gtest_main pthread)
add_test(UnitTest_Percolator_RunAllTests gtest_unit)

# Important to use relative paths here (used by CPack)!
install(TARGETS gtest_unit EXPORT PERCOLATOR DESTINATION ./bin)
