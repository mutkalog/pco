#!/bin/bash

ls . 

if [ $? -ne 0 ]; then
    exit 2
fi

echo -n "prepare.sh: here is PCO_STAGING_ARTIFACTS_PATHS:"
echo "$PCO_STAGING_ARTIFACTS_PATHS"

echo "prepare.sh: exiting"

exit 0
