# Queueing System Simulator  

This project simulates an **M/M/1** and **M/M/n** queueing system using **POSIX threads (pthreads)** in C. It models customer arrivals, queueing, and service times based on **exponential distributions**.  

## Features  
- **M/M/1 & M/M/n Simulation**: Supports up to 5 servers.  
- **Threaded Design**: Uses three threads for customer generation, service, and queue observation.  
- **Exponential Timing**: Arrival and service times follow an exponential distribution.  
- **Performance Metrics**: Computes mean, standard deviation, and server utilization.  
- **Command-Line Arguments**: Configure arrival rate, service rate, number of customers, and servers.  

## Usage  
Compile and run:  
```sh
gcc -o queue_sim queue_sim.c -lpthread -lm  
./queue_sim -l 5.0 -m 7.0 -c 1000 -s 1  
```

### Arguments  
- `-l lambda` (Arrival rate, default: **5.0**)  
- `-m mu` (Service rate, default: **7.0**)  
- `-c numCustomer` (Total customers, default: **1000**)  
- `-s numServer` (Number of servers **(1-5)**, default: **1**)  

## Output  
The simulator prints:  
- **Mean & Std Dev** of inter-arrival, waiting, service times, and queue length.  
- **Server Utilization**: Busy time / Total time.  

## Implementation Details  
- **Thread 1**: Generates customers, inserting them into the queue.  
- **Thread 2**: Fetches and serves customers.  
- **Thread 3**: Observes and records queue length.  
- **Synchronization**: Uses **mutexes & condition variables** to manage concurrency.  

## Notes  
- Uses `nanosleep()` for fractional-second timing.  
- Implements **reentrant random number generation** with `srand48_r()`.  
- Expands to **multiple servers** by spawning additional server threads.  
```
