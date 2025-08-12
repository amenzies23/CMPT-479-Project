rm -rf ./coverage

mkdir -p ./coverage
./test_calculator --gtest_output=json:./coverage/results.json

./test_calculator --gtest_list_tests | awk '
  /^[^ ]/ {suite=$1}
  /^[ ]/ {print suite substr($0,3)}
' | while read test; do
  find . -name '*.gcda' -delete
  ./test_calculator --gtest_filter="$test"
  mkdir -p "./coverage/$test"
  gcov ./CMakeFiles/calculator.dir/src/calculator.cpp.o 
  mv *.gcov "./coverage/$test/"
done