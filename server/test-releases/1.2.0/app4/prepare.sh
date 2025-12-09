#!/bin/bash

ls . 
if [ $? -ne 0 ]; then
    exit 2
fi

echo "Doing some scripty stuff!"
echo "Killing processes from that script!"

exit 0
