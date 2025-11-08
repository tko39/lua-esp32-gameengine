#!/bin/bash

if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <input_lua_file> mode"
    echo "Example: $0 ./bouncingBall.lua header"
    echo "Example: $0 ./bouncingBall.lua luac"
    echo "Note: If you are using `mode=luac`, make sure luac32 is in the same directory as this script. Integers and numbers have to be 32 bit for the ESP32."
    exit 1
fi

input_file="$1"
mode="$2"
exeDir="$(dirname "$0")"

if [ "$mode" == "header" ]; then
    output_file="$exeDir/../include/luaScript.h"
    echo "// Auto-generated header file from $input_file" > "$output_file"
    echo "#ifndef LUASCRIPT_H" >> "$output_file"
    echo "#define LUASCRIPT_H" >> "$output_file"
    echo "" >> "$output_file"
    echo "const char lua_script[] = R\"(" >> "$output_file"
    cat "$input_file" >> "$output_file"
    echo ")\";" >> "$output_file"
    echo "" >> "$output_file"
    echo "#endif // LUASCRIPT_H" >> "$output_file"
elif [ "$mode" == "luac" ]; then
    output_file="$exeDir/../data/runtime.luac"
    $exeDir/luac32 -s -o "$output_file" "$input_file"
else
    echo "Unsupported mode: $mode"
    exit 1
fi

