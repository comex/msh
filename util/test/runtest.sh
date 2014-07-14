#!/bin/bash
set -e
cd "$(dirname "${BASH_SOURCE[0]}")"
TESTS="misctest.c vectest.c"
if [ -n "$CC" ]; then
   CCS=("$CC")
else
   CCS=("/usr/local/Cellar/llvm/3.4.1/bin/clang -fsanitize=undefined"
      "gcc-4.8"
   )
fi
for file in $TESTS; do
   for cc in "${CCS[@]}"; do
      $cc -O3 -I../.. -o thetest $file
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
         bad=1
      fi
      expected_ec="$(awk '/^expect-exit/{print $2}' $file)"
      if [ "$actual_ec" != "$expected_ec" ]; then
         echo "$file exit differs.  Actual: $actual_ec  Expected: $expected_ec"
         bad=1
      fi
      -z $bad
   done
done

echo "All OK"
