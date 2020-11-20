# Copyright (c) 2020 Tom Deakin
# SPDX-License-Identifier: MIT

hex8asm: hex8asm.cpp hex8asm.hpp
	g++ -std=c++14 hex8asm.cpp -o $@ -O3 -Wall

.PHONY: clean
clean:
	rm -f hex8asm

