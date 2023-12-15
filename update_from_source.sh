#!/bin/bash
FUCHSIA_SHA=3907f2a281eb3fc971312c080eeefeb4fd8e7e04
URL=https://fuchsia.googlesource.com/fuchsia/+archive/$FUCHSIA_SHA/sdk/lib.tar.gz
wget $URL -O - | tar -xzvf - &&
	echo "# This is part of the Google Fuchsia SDK, use ./update_fron_source.sh to pull a new version." > README.md
