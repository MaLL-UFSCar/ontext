main:
	g++ -std=c++11 -O3 src/ontext.cpp -o output/ontext -fopenmp

run:
	output/ontext input/Filt-Relations15 input/Cat-Instances input/sample4m-sorted
