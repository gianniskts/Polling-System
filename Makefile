# Compiler
CXX = g++

# Compiler flags
CXXFLAGS = -std=c++14 -Wall -Wextra

# Shell
SH = /bin/bash

# Source and bin directories
SRC_DIR = src
BIN_DIR = bin
SCRIPT_DIR = scripts

# Targets
.PHONY: all poller pollSwayer create_input tally_votes process_log clean

all: directories poller pollSwayer create_input tally_votes process_log

directories: ${BIN_DIR}

${BIN_DIR}:
	mkdir -p ${BIN_DIR}

poller: $(SRC_DIR)/poller.cpp
	$(CXX) $(CXXFLAGS) $< -o $(BIN_DIR)/$@

pollSwayer: $(SRC_DIR)/pollSwayer.cpp
	$(CXX) $(CXXFLAGS) $< -o $(BIN_DIR)/$@

create_input: $(SCRIPT_DIR)/create_input.sh
	chmod +x $<

tally_votes: $(SCRIPT_DIR)/tallyVotes.sh
	chmod +x $<

process_log: $(SCRIPT_DIR)/processLogFile.sh
	chmod +x $<

clean:
	rm -rf $(BIN_DIR)
