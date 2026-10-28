#!/bin/bash

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

pip3 install --user git+https://github.com/supposedly/nutshell.git
pip3 install --user --upgrade tqdm

set -e

python3 -c 'import nutshell'
python3 -m unittest lifelib.tests.test_nutshell
