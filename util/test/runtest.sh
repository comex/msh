#!/bin/bash
set -e
cd "$(dirname "${BASH_SOURCE[0]}")"
if [ -n "$CC" ]; then
   CCS=("$CC")
else
   CCS=("/usr/local/Cellar/llvm/3.4.1/bin/clang -fsanitize=undefined"
      "gcc-4.8"
   )
fi
if [ -n "$1" ]; then
   for file in "$@"; do
      for cc in "${CCS[@]}"; do
         thetest=thetest.$$
         CMD=($cc -std=gnu11 -O3 -Wall -Wextra -Werror -o $thetest $file)
         echo "${CMD[*]}"
         "${CMD[@]}"
         set +e
         actual="$(./$thetest)"
         actual_ec=$?
         set -e
         rm ./$thetest
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
         test -z $bad
      done
   done
else
   echo 'misctest.c vectest.c alstacktest.c strtest.c timetest.c chantest.c tinycthreadtest.c' | xargs -P 4 -n 1 ./runtest.sh
   echo "All OK"
fi
