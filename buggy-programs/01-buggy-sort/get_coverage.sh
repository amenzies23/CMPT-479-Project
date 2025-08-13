rm -rf ./coverage

mkdir -p ./coverage
./test_sort --gtest_output=json:./coverage/results.json

./test_sort --gtest_list_tests | awk '
  /^[^ ]/ {suite=$1}
  /^[ ]/ {print suite substr($0,3)}
' | while read test; do
  find . -name '*.gcda' -delete
  ./test_sort --gtest_filter="$test"
  mkdir -p "./coverage/$test"
  for src in sort.cpp; do
    gcov ./CMakeFiles/sort_lib.dir/src/$src.o
     mv ${src}.gcov "./coverage/$test/"
  done
done