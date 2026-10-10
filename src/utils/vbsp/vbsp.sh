#!/bin/bash

current_path=$( cd "$(dirname "${BASH_SOURCE[0]}")" ; pwd -P )

LD_LIBRARY_PATH="$current_path" "$current_path/vbsp" -game "$current_path/../../neo" "$@"
