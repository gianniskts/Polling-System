#!/bin/bash

# Check number of input arguments
if [ "$#" -ne 2 ]; then # checks if the number of arguments is different from 2
    echo "Usage: ./create_input.sh politicalParties.txt numLines"
    exit 1
fi

# Assigning arguments to variables
parties_file="../data/$1" # political parties file which is located in the data folder
num_lines=$2 # number of lines to be generated

# Check if the political parties file exists
if [ ! -f $parties_file ]; then
    echo "File $parties_file not found"
    exit 1
fi

# Generating random names, surnames and parties
for (( i=0; i<$num_lines; i++ ))
do
    # first letter is always capitalized, head is used to limit the length of the name, sed is used to capitalize the first letter, tr is used to generate random letters
    firstName=$(head /dev/urandom | tr -dc A-Za-z | head -c $((RANDOM%10+3)) | sed -e "s/\b\(.\)/\u\1/g") # random first name with length between 3 and 12, 
    surname=$(head /dev/urandom | tr -dc A-Za-z | head -c $((RANDOM%10+3)) | sed -e "s/\b\(.\)/\u\1/g") # random surname with length between 3 and 12
    party=$(shuf -n 1 $parties_file) # random party from the political parties file
    echo "$firstName $surname $party" >> "../data/inputFile.txt" # writing the generated data to the input file
done
