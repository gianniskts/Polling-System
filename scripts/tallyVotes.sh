#!/bin/bash

# Check if inputFile.txt exists
if [ ! -f "../data/inputFile.txt" ]; then
    echo "inputFile.txt not found!"
    exit 1
fi

# Check if a tally file name is provided
if [ -z "$1" ]; then
    echo "Please provide a filename to store tally results."
    exit 1
fi

# Process inputFile.txt to count the first vote of each unique voter
awk '!seen[$1]++ {print $2}' "../data/inputFile.txt" | sort | uniq -c > "../results/$1"
