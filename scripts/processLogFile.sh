#!/bin/bash

# Check if the number of arguments is correct
if [ $# -ne 1 ]; then
    echo "Usage: ./processLogFile.sh poll-log"
    exit 1
fi

# Variables
poll_log=$1
poller_results_file="pollerResultsFile.txt"

# Check if poll-log exists and has read permissions
if [ ! -f "../logs/$poll_log" ] || [ ! -r "../logs/$poll_log" ]; then
    echo "Poll log file does not exist or doesn't have read permissions."
    exit 1
fi

# Reading poll-log and tally the votes
awk 'BEGIN {OFS=FS=" "; total = 0} !seen[$1]++ {votes[$2]++; total++} END {for (i in votes) print i, votes[i]; print "TOTAL", total}' "../logs/$poll_log" > "../results/$poller_results_file"
