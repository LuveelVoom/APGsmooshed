#!/bin/bash

# This allows a pathless Cygwin bash to bootstrap its way into finding
# a compiler:

export PATH="/usr/local/bin:/usr/bin:/bin:$PATH"

cd "$( dirname "${BASH_SOURCE[0]}" )"
export CHERE_INVOKING=1
. /etc/profile

"$@"
