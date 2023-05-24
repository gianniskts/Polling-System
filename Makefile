# Compiler
CXX = g++

# Compiler flags
CXXFLAGS = -std=c++14 -Wall -Wextra

# Shell
SH = /bin/bash

# Targets
all: poller pollSwayer create_input tally_votes process_log

poller: ../src/poller.cpp
	$(CXX) $(CXXFLAGS) $< -o ../bin/$@

pollSwayer: ../src/pollSwayer.cpp
	$(CXX) $(CXXFLAGS) $< -o ../bin/$@

create_input: ../scripts/create_input.sh
	cp $< ../bin/
	chmod +x ../bin/$@

tally_votes: ../scripts/tallyVotes.sh
	cp $< ../bin/
	chmod +x ../bin/$@

process_log: ../scripts/processLogFile.sh
	cp $< ../bin/
	chmod +x ../bin/$@

clean:
	rm -f ../bin/poller ../bin/pollSwayer ../bin/create_input ../bin/tally_votes ../bin/process_log
