set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/tests")

function(calculator_gcov_tests TEST_SRC TEST_GROUP_NAME LIBRARIES)
  set(test_file "${CMAKE_CURRENT_SOURCE_DIR}/${TEST_SRC}.cpp")
  set(test_name "${TEST_GROUP_NAME}")

  add_executable(${test_name} ${test_file})
  target_link_libraries(${test_name} PRIVATE ${LIBRARIES})
  target_include_directories(${test_name} PRIVATE ${CMAKE_SOURCE_DIR}/include)

  add_test(NAME "${test_name}" COMMAND ${test_name})
  set_tests_properties("${test_name}" PROPERTIES LABELS "${TEST_GROUP_NAME}")

  set(COVERAGE_DIR ${CMAKE_BINARY_DIR}/coverage/${test_name})

add_custom_target(gcov_${test_name}
  COMMAND mkdir -p ${COVERAGE_DIR}
  COMMAND find ${CMAKE_BINARY_DIR} -name "*.gcda" -delete # need to clear the coverage data
  COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${test_name} || echo "Test completed"
  COMMAND gcov -b ${CMAKE_SOURCE_DIR}/src/calculator.cpp -o ${CMAKE_BINARY_DIR}/CMakeFiles/calculator_lib.dir/src
  COMMAND cp calculator.cpp.gcov ${COVERAGE_DIR}/
  COMMAND rm -f *.gcov
)

  if(NOT TARGET gcov_all)
    add_custom_target(gcov_all
      COMMAND ctest
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
  endif()

  add_dependencies(gcov_all gcov_${TEST_GROUP_NAME})

endfunction(calculator_gcov_tests)

add_custom_target(gcov_clean
  COMMAND rm -rf ${CMAKE_BINARY_DIR}/coverage
  COMMAND rm -rf ${CMAKE_BINARY_DIR}/Testing/Temporary
  COMMAND rm -rf ${CMAKE_BINARY_DIR}/Testing
  COMMAND find ${CMAKE_BINARY_DIR} -name "*.gcda" -delete
  COMMAND find ${CMAKE_BINARY_DIR} -name "*.gcno" -delete
  COMMAND find ${CMAKE_BINARY_DIR} -name "*.gcov" -delete
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

file(GLOB TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp")
foreach(test_src ${TEST_SOURCES})
    get_filename_component(test_name ${test_src} NAME_WE)
    string(REPLACE "test_" "" test_group_name ${test_name})
    calculator_gcov_tests("${test_name}" "${test_group_name}" "calculator_lib")
endforeach()