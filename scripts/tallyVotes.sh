#!/bin/bash

# Check if inputFile.txt exists
if [ ! -f "../data/inputFile.txt" ]; then # checks if the file inputFile.txt does not exist
    echo "inputFile.txt not found!"
    exit 1
fi

# Check if a tally file name is provided
if [ -z "$1" ]; then 
    echo "Please provide a filename to store tally results."
    exit 1
fi

# Process inputFile.txt to count the first vote of each unique voter
# awk is used to process text files, the BEGIN block is executed before the first input line is read, the END block is executed after the last input line is read, the OFS variable is used to set the output field separator, the FS variable is used to set the field separator, the length function is used to get the length of a string, the substr function is used to get a substring of a string, the ++ operator is used to increment a variable, the in operator is used to check if an element is in an array, the print function is used to print the results, the for loop is used to iterate over the elements of an array
awk '!seen[substr($0, 1, length($0) - length($NF))]++ {print $NF}' "../data/inputFile.txt" | sort | uniq -c | awk '{ printf("%d %s\n", $1, $2) }' > "../results/$1" # tallying the votes and writing the results to the poller results file 
