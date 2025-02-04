# cpp
CCFLAGS = -std=c++11 -Wall -g -Wno-unknown-pragmas
CC = g++
C_OUTPUT_DIR = _bin
# wasm
EXPORTED_FUNCTIONS = ["_malloc", "_free"]
WCFLAGS = -s WASM=1 -O3 -s EXPORTED_FUNCTIONS='$(EXPORTED_FUNCTIONS)'
WCC = em++
W_OUTPUT_DIR = docs/wasm

chess:
	@mkdir -p $(C_OUTPUT_DIR)
	@$(CC) ConsoleChess.cpp engine/*.cpp engine/*/*.cpp $(CCFLAGS) -o $(C_OUTPUT_DIR)/chess
	@echo "Final file size:"
	@du -h $(C_OUTPUT_DIR)/chess

wasm: wasm/* engine/*.cpp engine/*/*.cpp
	@mkdir -p $(W_OUTPUT_DIR)
	@$(WCC) wasm/Main.cpp engine/*.cpp engine/*/*.cpp $(WCFLAGS) -o $(W_OUTPUT_DIR)/chess.js
	@cat wasm/*.js >> $(W_OUTPUT_DIR)/chess.js
	@echo "Final file sizes:"
	@du -h $(W_OUTPUT_DIR)/chess.js
	@du -h $(W_OUTPUT_DIR)/chess.wasm

clean:
	@rm -rf $(C_OUTPUT_DIR)
	@rm -rf $(W_OUTPUT_DIR)