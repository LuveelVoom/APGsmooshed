#!/bin/bash

if [ -z "$*" ]; then
"$0" "python3"
exit $?
fi

cd "$( dirname "${BASH_SOURCE[0]}" )"
cd ..

lifelib_basename="$( basename "$( pwd )" )"

if [ "$lifelib_basename" = "lifelib" ]; then
echo "Directory has the correct name."
else
echo "Directory must be called 'lifelib', not '$lifelib_basename'."
exit 1
fi

cd ..

echo "Checking to see whether both versions of Python are installed..."

python_exists="true"
pythons=()

for i in "$@"; do

    if [[ "$i" =~ ^[2-9][0-9.]*$ ]]; then
        python_name="python$i"
    else
        python_name="$i"
    fi

    python_version="$( "$python_name" --version 2>&1 )"
    python_status="$?"

    if [ "$python_status" = 0 ]; then
        printf "$python_name version: \033[1;32m$python_version\033[0m\n"
        pythons+=("$python_name")
    else
        printf "$python_name exited with status code $python_status: \033[1;31m$python_version\033[0m\n"
        python_exists="false"
    fi

done

if [ "$python_exists" = "false" ]; then
    exit 1
fi

printf "Removing old .so files...\n"
rm lifelib/pythlib/*.so

printf "Running tests...\n\n"
set -e

for testtype in "unit" "integration" "indirection"; do
    printf "\n\033[1;36m **** $testtype tests ****\033[0m\n"
    for python_name in "${pythons[@]}"; do
        "$python_name" -m unittest discover "lifelib/tests/$testtype"
    done
done

printf "\n\033[1;36m **** segfault tests ****\033[0m\n"
for python_name in "${pythons[@]}"; do
    "$python_name" -c 'import lifelib; lt = lifelib.load_rules("b3s23").lifetree(); pat = lt.pattern(); f = lambda x : x'
done

printf "\n\033[1mAll tests completed successfully.\033[0m\n"
