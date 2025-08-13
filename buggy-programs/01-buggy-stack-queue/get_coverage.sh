rm -rf ./coverage

mkdir -p ./coverage
./test_stack_queue --gtest_output=json:./coverage/results.json

./test_stack_queue --gtest_list_tests | awk '
  /^[^ ]/ {suite=$1}
  /^[ ]/ {print suite substr($0,3)}
' | while read test; do
  find . -name '*.gcda' -delete
  ./test_stack_queue --gtest_filter="$test"
  mkdir -p "./coverage/$test"
  for src in stack-queue.cpp; do
    gcov ./CMakeFiles/stack_queue_lib.dir/src/$src.o
     mv ${src}.gcov "./coverage/$test/"
  done
done