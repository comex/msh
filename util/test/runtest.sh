#!/bin/bash
set -e
[ -z "$CC" ] && CC="/usr/local/Cellar/llvm/3.4.1/bin/clang -fsanitize=undefined"
for file in misctest.c vectest.c; do
   $CC -O3 -I.. -o thetest $file
   set +e
   actual="$(./thetest)"
   actual_ec=$?
   set -e
   expected="$(awk '/^>>/{on=0} on{print} /^expect-output<</{on=1}' $file)"
   if [ "$actual" != "$expected" ]; then
      echo "$file output differs.  Actual:"
      echo "$actual"
      echo "Expected:"
      echo "$expected"
      echo
   fi
   expected_ec="$(awk '/^expect-exit/{print $2}' $file)"
   if [ "$actual_ec" != "$expected_ec" ]; then
      echo "$file exit differs.  Actual: $actual_ec  Expected: $expected_ec"
   fi
done

echo "All OK"
