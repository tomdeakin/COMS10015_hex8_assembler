hex8asm: *.cpp *.hpp
	g++ -std=c++14 *.cpp -o $@ -O3 -Wall
	
