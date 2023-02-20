build:
	mpic++ -o tema3 tema3.cpp -Wall -lm

run:
	mpirun --oversubscribe -np 15 ./tema3 150 150

clean:
	rm -rf tema3
