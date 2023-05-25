# Polling System

This is a polling system consisting of a server and client program. The server allows clients to connect and vote for political parties, while the client program sends votes to the server. Additionally, there are several scripts provided to assist with data generation and result processing.

## Directory Structure

Project2
│   README.md        // add a readme file to describe the project
│   Makefile         // add your makefile here
│
└───src              // Source files (C++)
│   │   poller.cpp
│   │   pollSway.cpp
│   
└───scripts          // Shell scripts
│   │   create_input.sh
│   │   processLogFile.sh
│   │   tallyVotes.sh
│
└───data             // Data files
│   │   politicalParties.txt
│   │   inputFile.txt
│   │   votes.txt
│
└───logs             // Log files
│   │   pollLog.txt
│   │   pollStats.txt
│   
└───results          // Results files
│   │   pollerResultsFile.txt
│   │   tallyResultsFile.txt
│   
└───.vscode          // Configuration files for Visual Studio Code


- `.vscode`: Configuration files for Visual Studio Code.
- `data`: Directory to store data files.
- `logs`: Directory to store log files.
- `results`: Directory to store result files.
- `scripts`: Bash scripts for data generation and processing.
- `src`: Source code files.
- `Makefile`: Makefile for building the project.
- `README.md`: This README file.

## Server

The server is implemented in C++ and uses multithreading to handle multiple client connections simultaneously. It keeps track of the votes received from clients and provides a poll log and poll stats.

### Files

- `src/poller.cpp`: Server implementation file.
- `src/poller.h`: Server header file.

### Usage

To run the server, use the following command:

./bin/poller [portnum] [numWorkerthreads] [bufferSize] [poll-log] [poll-stats]

- `portnum`: The port number to run the server on.
- `numWorkerthreads`: The number of worker threads to use for processing client requests.
- `bufferSize`: The size of the connection buffer.
- `poll-log`: The file to log the votes received from clients.
- `poll-stats`: The file to store the poll statistics.

## Client

The client program is implemented in C++ and can be used to send votes to the server. It takes the server name, port number, and an input file containing the names and votes of voters as command line arguments.

### Files

- `src/pollSwayer.cpp`: Client implementation file.
- `src/pollSwayer.h`: Client header file.

### Usage

To run the client, use the following command:

./bin/pollSwayer [serverName] [portNum] [inputFile.txt]


- `serverName`: The name of the server to connect to.
- `portNum`: The port number that the server is listening on.
- `inputFile.txt`: A file containing names and votes of voters.

## Scripts

This project also includes several scripts to assist with testing and processing the poll results.

### Files

- `scripts/create_input.sh`: Generates an input file with random names and votes based on a list of political parties.
- `scripts/tallyVotes.sh`: Processes the input file to tally the votes for each party and store the results in a file.
- `scripts/processLogFile.sh`: Processes the poll log file to count the votes for each party and store the results in a file.

### Usage

To run the scripts, navigate to the project root directory and execute them using the following commands:

- `./scripts/create_input.sh politicalParties.txt numLines`
- `./scripts/tallyVotes.sh tallyResultsFile.txt`
- `./scripts/processLogFile.sh poll-log`

Make sure to provide the required arguments for each script.

## Getting Started

1. Clone the repository:

