<p align="justify">
  
# Collaborative-Computing-in-Distributed-Systems

This is the implementation of a distributed program in MPI where processes are grouped into a topology consisting of four clusters, each with a coordinator and an arbitrary number of worker processes. Worker processes in a cluster can only communicate with their coordinator, and the four coordinators can communicate with each other in a ring topology to connect the clusters. 

The project's goal is for all worker processes to work together, with the help of coordinators, to solve computational tasks. This will be achieved by establishing the topology and disseminating it to all processes, and then by dividing the calculations as evenly as possible among the workers. <br>

### Establishing Topology

The distributed system implemented in this project consists of four clusters with an arbitrary number of worker processes, as shown in the figure below. Each process knows that there are four clusters from the beginning of the program's execution.

![1](https://user-images.githubusercontent.com/73998092/220163845-ffc5f7ac-caee-4c38-8566-1078c5746cf5.PNG)

Each cluster has a coordinator, i.e., processes with ranks 0, 1, 2, and 3 in the implementation (this being known from the start by all coordinators). Each coordinator is responsible for its own worker processes, as can be seen in the figure above (where each line between two processes represents a communication channel).

At the beginning of the distributed program's execution, the coordinator processes read information about the processes in their clusters from four input files (one for each coordinator), and a worker process does not know which rank the coordinator of its cluster is in. Therefore, it is the cluster coordinators' responsibility to inform their workers about who their coordinator is.

The first task is to establish the topology of the distributed system in all processes. At the end of this task, each process (whether it is a coordinator or a worker) must know the entire system's topology and display it on the screen. Each message directly sent by a process must be logged in the terminal as M(0,4) (process 0 sends a message to process 4). <br>


### Performing Calculations

Once all processes know the topology, the calculation part, which is coordinated by process 0, follows. Specifically, process 0 generates a vector V of size N (where N is received as a parameter when running the program and is initially only known by process 0), where V[K] = N - K - 1 (with K between 0 and N-1), which must be multiplied by 5 element-wise. Multiplying the vector's elements by 5 is only performed by worker processes, so it is the coordinators' responsibility to divide the calculations as evenly as possible among the worker processes. <br>

### Handling Communication Channel Failures

The third part of the assignment involves handling the case when there is an error on the communication channel between processes 0 and 1. Specifically, the connection between the two processes disappears, as shown in the image below (where we have the same structure of the distributed system presented earlier).

![2](https://user-images.githubusercontent.com/73998092/220164054-3720302b-249b-40c7-9070-f990215445f7.PNG)

Thus, the previous requirements (establishing the topology in all processes and then multiplying the vector's elements by 5) must be solved in the absence of the connection between processes 0 and 1. <br>

### Execution

Compiling results in an MPI binary that runs as follows:

<mpirun --oversubscribe -np <num_processes> ./executable <vector_size> <communication_error>>

The second parameter given at runtime will be 0 if the connection between processes 0 and 1 exists (there is no communication error) or 1 if processes 0 and 1 cannot communicate directly. <br>


### Implementation details

To solve the requirement where there is no direct connection between processes 0 and 1, I thought of designing the connections from the beginning in such a way that process 0 cannot communicate directly with process 1.

In the process of finding the topology, I used a structure consisting of four vectors, one for each process in the ring.

&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;* Sent this structure from 0 to 3, from 3 to 2, and from 2 to 1. <br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;* After each process received the structure, it added all other processes in its own cluster to the corresponding vector of its rank and sent the updated structure further. <br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;* After updating its vector, process 1 sent the complete structure back to 2 and to the processes in its own cluster. <br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;* Each process received the complete structure, displayed it, sent it to the processes in its own cluster, and to the neighboring process in the ring. <br>

In the calculation process, I proceeded as follows:

&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;* Process 0 created the vector and distributed it according to the number of workers (calculated using the topology). <br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;* It kept the index of the vector corresponding to the sub-processes in its cluster and sent the initial vector to process 3, along with the following information: <br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;- the index from which 3 can distribute elements from the vector for sub-processes <br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;- the number of elements each process needs to perform calculations <br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;- the plus variable, which can allocate an additional element for the worker to ensure an even distribution (remainder of division) <br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;* Each leader kept the index for its sub-processes and sent the initial vector to both its sub-processes and the next sub-process. <br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;* Finally, leader 1 waits for the results from its sub-processes and puts them in the initial vector at the specific index. Furher It sends the initial vector, modified only in its allocated area, to process 2. <br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;* Process 2 receives the vector from 1, modifies its specific area in this vector, and sends it to 3, which follows the same steps. <br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;* Process 0 modifies its area in the vector and displays the final result. <br>
  </p>
