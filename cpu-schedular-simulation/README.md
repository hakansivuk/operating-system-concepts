# CPU Schedular Simulation

This C application simulates a CPU in a multi-threaded environment. The main process is called SE thread and it generates threads that are called PS threads. Each PS thread represents a process and they sent _jobs_ representing CPU bursts to the SE thread which essentially simulates a CPU. The SE thread executes CPU bursts according to the job and the regarding PS thread sleeps for waiting I/O. 

Three types of scheduling are supported: first-come-first-serve (FCFS), shortest-job-first (SJF, non-preemptive), round-robin (RR). The burst and I/O time are in the order of 100. The simulation can be made totally random or custom data could be given to the simulator.

## How to Run

> Compile the application using the Make file.
> Execute by:
> ./schedule N minCPU maxCPU minIO maxIO outfile duration algorithm quantum infileprefix

* N is the number of PS threads.
* minCPU is the minimum CPU burst length and maxCPU is the maximum CPU burst length, that a process can have. Similarly, minIO is the minimum I/O wait time, and maxIO is the maximum I/O wait time, that a process can have.
* If infileprefix is equal to “no-infile” string, cpu burst times are randomly generated (uniformly distributed between minCPU and maxCPU). Similarly, io-burst (i.e., io-wait) times are randomly generated (uniformly distributed between minIO and maxIO).
* If infileprefix is something different than “no-infile”, cpu and io burst lengths would be obtained from an input file (the name of the input file will have the specified prefix) for each PS thread. The input file for PS thread X (1<=X<=N) must have a name with the format “infileprefixX.txt”. For example: if infileprex is “infile”, then the input filename for simulated process 4 must be “infile4.txt”.
* Duration determines how long the simulation will last. If set to a value M, then each PS thread would generate M cpu bursts. For example, M can be “100”.
* \<alg\> is the scheduling algoritm to simulate. It can be “FCFS”, “SJF”, or “RR”. If FCS or SJF is specified, then q must be set to 0; if RR is specified, then q is a value greater than 100 and less than 300. The q value can be, for example, 100.
* The outfile parameter is the name of the output file generated.

* If input is to be taken from files, the format of each input file must be like the following. First burst type is specified, then the length of burst (in ms):
	CPU 200  
	IO 600
	CPU 300 
	IO 800 
    CPU 200

**An example invocation of the program can be like the following:**
> schedule 5 100 300 500 8000 out.txt 50 RR 100 no-infile

For further technical details, you could refer to _project2.pdf_ file.
