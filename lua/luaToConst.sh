#!/bin/bash

usage() {
    echo "Usage: $0 -m <mode> <input_lua_file(s)>"
    echo "  -m <mode>    Mode: header or luac"
    echo "Example: $0 -m header ./bouncingBall.lua"
    echo "Example: $0 -m header ./file1.lua ./file2.lua"
    echo "Example: $0 -m luac ./bouncingBall.lua"
    echo "Note: If you are using \`mode=luac\`, make sure luac32 is in the same directory as this script. Integers and numbers have to be 32 bit for the ESP32."
    exit 1
}

mode=""
while getopts ":m:" opt; do
    case $opt in
        m)
            mode="$OPTARG"
            ;;
        \?)
            usage
            ;;
    esac
done
shift $((OPTIND -1))

if [ -z "$mode" ] || [ "$#" -lt 1 ]; then
    usage
fi

input_files=("$@")
exeDir="$(dirname "$0")"

if [ "$mode" == "header" ]; then
    output_file="$exeDir/../include/luaScript.h"
    echo "// Auto-generated header file from ${input_files[*]}" > "$output_file"
    echo "#ifndef LUASCRIPT_H" >> "$output_file"
    echo "#define LUASCRIPT_H" >> "$output_file"
    echo "" >> "$output_file"
    echo "const char* lua_scripts[] = {" >> "$output_file"
    for input_file in "${input_files[@]}"; do
        if [ ! -f "$input_file" ]; then
            echo "Error: Input file '$input_file' does not exist."
            exit 1
        fi

        if ! $exeDir/luac32 -p "$input_file"; then
            echo "Error: luac32 failed to compile '$input_file'."
            exit 1
        fi

        echo "  R\"(" >> "$output_file"
        cat "$input_file" | sed -e "s/    /  /g" -e "s/--.*$//g" | grep -v "^\s*$" >> "$output_file"
        echo ")\"" >> "$output_file"
        echo "  ," >> "$output_file"
    done
    echo "  nullptr" >> "$output_file"
    echo "};" >> "$output_file"
    echo "" >> "$output_file"

    echo "const char* script_names[] = {" >> "$output_file"
    for input_file in "${input_files[@]}"; do
        filename=$(basename "$input_file")
        filename="${filename%.*}"
        # Split camel case and capitalize each word
        formatted_name=$(echo "$filename" | sed -E 's/([a-z])([A-Z])/\1 \2/g' | awk '{for(i=1;i<=NF;i++){$i=toupper(substr($i,1,1)) substr($i,2)}}1')
        filename="$formatted_name"
        echo "  \"$filename\"," >> "$output_file"
    done

    echo "  nullptr" >> "$output_file"
    echo "};" >> "$output_file"
    echo "" >> "$output_file"
    echo "#endif // LUASCRIPT_H" >> "$output_file"
elif [ "$mode" == "luac" ]; then
    if [ "${#input_files[@]}" -ne 1 ]; then
        echo "Error: Only one input file allowed in luac mode."
        exit 1
    fi
    output_file="$exeDir/../data/runtime.luac"
    $exeDir/luac32 -s -o "$output_file" "${input_files[0]}"
else
    echo "Unsupported mode: $mode"
    exit 1
fi

