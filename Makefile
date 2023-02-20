build:
	mpic++ -o program DistributedSystems.cpp -Wall -lm

run:
	mpirun --oversubscribe -np 15 ./program 150 150

clean:
	rm -rf program
