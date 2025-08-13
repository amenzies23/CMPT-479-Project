rm -rf ./coverage

mkdir -p ./coverage
./test_area_calculator --gtest_output=json:./coverage/results.json

./test_area_calculator --gtest_list_tests | awk '
  /^[^ ]/ {suite=$1}
  /^[ ]/ {print suite substr($0,3)}
' | while read test; do
  find . -name '*.gcda' -delete
  ./test_area_calculator --gtest_filter="$test"
  mkdir -p "./coverage/$test"
  for src in rectangle.cpp circle.cpp triangle.cpp area_calculator.cpp; do
    gcov ./CMakeFiles/area_lib.dir/src/$src.o
    mv ${src}.gcov "./coverage/$test/"
  done
done