rm -rf ./coverage

mkdir -p ./coverage
./test_linked_list --gtest_output=json:./coverage/results.json

./test_linked_list --gtest_list_tests | awk '
  /^[^ ]/ {suite=$1}
  /^[ ]/ {print suite substr($0,3)}
' | while read test; do
  find . -name '*.gcda' -delete
  ./test_linked_list --gtest_filter="$test"
  mkdir -p "./coverage/$test"
  for src in linked_list.cpp; do
    gcov ./CMakeFiles/linked_list_lib.dir/src/$src.o
  done
  mv *.gcov "./coverage/$test/"
done