#!/bin/bash

function indent_c () {
    echo "indent $1 ..."
    indent $1 -o $1
}

for i in $( ls *.c ); do
    indent_c $i
done
