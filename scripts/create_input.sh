#!/bin/bash

# Check number of input arguments
if [ "$#" -ne 2 ]; then
    echo "Usage: ./create_input.sh politicalParties.txt numLines"
    exit 1
fi

# Assigning arguments to variables
parties_file="../data/$1"
num_lines=$2

# Check if the political parties file exists
if [ ! -f $parties_file ]; then
    echo "File $parties_file not found"
    exit 1
fi

# Generating random names, surnames and parties
for (( i=0; i<$num_lines; i++ ))
do
    name=$(head /dev/urandom | tr -dc A-Za-z | head -c $((RANDOM%10+3)))
    party=$(shuf -n 1 $parties_file)
    echo "$name $party" >> "../data/inputFile.txt"
done
