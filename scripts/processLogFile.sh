#!/bin/bash

# Check if the number of arguments is correct
if [ $# -ne 1 ]; then # if the number of arguments is different from 1
    echo "Usage: ./processLogFile.sh poll-log"
    exit 1
fi

# Variables
poll_log=$1 # poll log file
poller_results_file="pollerResultsFile.txt" # poller results file

# Check if poll-log exists and has read permissions
if [ ! -f "../logs/$poll_log" ] || [ ! -r "../logs/$poll_log" ]; then # if the poll-log file doesn't exist or doesn't have read permissions
    echo "Poll log file does not exist or doesn't have read permissions."
    exit 1
fi

# Reading poll-log and tally the votes
# awk is used to process text files, the BEGIN block is executed before the first input line is read, the END block is executed after the last input line is read, the OFS variable is used to set the output field separator, the FS variable is used to set the field separator, the length function is used to get the length of a string, the substr function is used to get a substring of a string, the ++ operator is used to increment a variable, the in operator is used to check if an element is in an array, the print function is used to print the results, the for loop is used to iterate over the elements of an array
awk 'BEGIN {OFS=FS=" "; total = 0} !seen[substr($0, 1, length($0) - length($NF))]++ {votes[$NF]++; total++} END {for (i in votes) print i, votes[i]; print "TOTAL", total}' "../logs/$poll_log" > "../results/$poller_results_file" # tallying the votes and writing the results to the poller results file