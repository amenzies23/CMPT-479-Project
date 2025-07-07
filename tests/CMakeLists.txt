set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/tests")

function(add_gcov_tests TEST_DIR TEST_GROUP_NAME LIBRARIES)
  file(GLOB files "${TEST_DIR}/*.cpp")
  list(GET files 0 test_file) # get the .cpp file
  get_filename_component(testcase ${test_file} NAME_WE)
  
  set(test_name "${TEST_GROUP_NAME}")
  
  add_executable(${test_name} ${test_file})
  target_link_libraries(${test_name} PRIVATE ${LIBRARIES})
  target_include_directories(${test_name} PRIVATE ${CMAKE_SOURCE_DIR}/include)
  
  add_test(NAME "${test_name}" COMMAND ${test_name})
  set_tests_properties("${test_name}" PROPERTIES LABELS "${TEST_GROUP_NAME}")
  
  set(OBJECT_DIR ${CMAKE_BINARY_DIR}/tests/CMakeFiles/${test_name}.dir) # .gcda files are saved in the test_* folder within the .dir
  
  add_custom_target(gcov_${test_name}
    COMMAND mkdir -p ${CMAKE_CURRENT_SOURCE_DIR}/${TEST_DIR}/coverage
    COMMAND find ${CMAKE_BINARY_DIR} -name "*.gcda" -delete # need to clear the coverage data
    COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${test_name} || echo "Test completed"
    COMMAND gcov -b ${CMAKE_SOURCE_DIR}/src/add.cpp -o ${CMAKE_BINARY_DIR}/CMakeFiles/add_lib.dir/src
    COMMAND cp add.cpp.gcov ${CMAKE_CURRENT_SOURCE_DIR}/${TEST_DIR}/coverage/
    COMMAND rm -f *.gcov
  )
  
  if(NOT TARGET gcov_all)
    add_custom_target(gcov_all
      COMMAND ctest # this will produce the .log files to know which tests fails
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
  endif()
  
  # ensures that all individual tests runs through the dependency
  add_dependencies(gcov_all gcov_${TEST_GROUP_NAME})
  
endfunction(add_gcov_tests)

add_custom_target(gcov_clean
  COMMAND rm -rf ${CMAKE_BINARY_DIR}/Testing
  COMMAND rm -rf ${CMAKE_CURRENT_SOURCE_DIR}/test_*/coverage
  COMMAND rm -f *.gcov || true
  COMMAND find ${CMAKE_BINARY_DIR} -name "*.gcda" -delete
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

add_gcov_tests("test_zero" "zero" "add_lib")
add_gcov_tests("test_positive" "positive" "add_lib")  
add_gcov_tests("test_negative" "negative" "add_lib")
add_gcov_tests("test_mixed" "mixed" "add_lib")
add_gcov_tests("test_with_zero" "with_zero" "add_lib")