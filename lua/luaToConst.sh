#!/bin/bash

if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <input_lua_file> <output_header_file>"
    echo "Example: $0 ./bouncingBall.lua ../include/luaScript.h"
    exit 1
fi

input_file="$1"
output_file="$2"

echo "// Auto-generated header file from $input_file" > "$output_file"
echo "#ifndef LUASCRIPT_H" >> "$output_file"
echo "#define LUASCRIPT_H" >> "$output_file"
echo "" >> "$output_file"
echo "const char lua_script[] = R\"(" >> "$output_file"
cat "$input_file" >> "$output_file"
echo ")\";" >> "$output_file"
echo "" >> "$output_file"
echo "#endif // LUASCRIPT_H" >> "$output_file"
