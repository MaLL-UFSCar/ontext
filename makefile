main:
	g++ -std=c++11 -O3 src/prontext2.cpp -o output/prontext2

run:
	output/prontext2 input/Filt-Relations15 input/Cat-Instances input/sample4m-sorted
