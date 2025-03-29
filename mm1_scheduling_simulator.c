#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
#include<sys/time.h>
#include<stdbool.h>
#include<time.h>
#include<getopt.h>
#include<math.h>

typedef struct customer customer;
typedef struct numberOfElementsInQueue numberOfElementsInQueue;

struct customer {
  struct timeval arrivalTime;
  customer *next;
};

customer *qHead, *qTail;
unsigned qLength;

struct numberOfElementsInQueue {
  unsigned numberOfElements;
  numberOfElementsInQueue *next;
};

numberOfElementsInQueue *nqHead, *nqTail;

double lambda = 5.0;
double mu = 7.0;
int numCustomer = 10;
int numServer = 1;

bool lastPrint = false;

pthread_mutex_t qMutex;
pthread_cond_t condition;

// Maintain a global variable which will act as a flag. Initially it will be false but when 1000th customer is processed it will turn true
bool lastCustomerFlag = false;

// Define arrays
double *interArrivalTimes;
double *waitingTimes;
double *serviceTimes;

// Keep count of sizes of each array i.e. index way
int indexInterArrivalTimes = 0;
int indexWaitingTimes = 0;
int indexServiceTimes = 0;

// Define variables for mean and standard deviation
double avgInterArrivalTimes=0.0, avgWaitingTimes=0.0, avgServiceTimes=0.0;
double stdInterArrivalTimes=0.0, stdWaitingTimes=0.0, stdServiceTimes=0.0;
double avgQueueLength=0.0;
double stdQueueLength=0.0;
double busyTime=0.0;

// Functions for calculating the mean and standard deviation of waiting times, inter arrival times and service times
void onlineAvgStd1(){
  double sumXInterArrivalTimes, sumX2InterArrivalTimes;
  double sumXWaitingTimes, sumX2WaitingTimes;
  double sumXServiceTimes, sumX2ServiceTimes;

  sumXInterArrivalTimes = sumX2InterArrivalTimes = 0.0;
  sumXWaitingTimes = sumX2WaitingTimes = 0.0;
  sumXServiceTimes = sumX2ServiceTimes = 0.0;

  for(int i=0; i<numCustomer; i++){
    sumXInterArrivalTimes += interArrivalTimes[i];
    sumX2InterArrivalTimes += interArrivalTimes[i] * interArrivalTimes[i];

    sumXWaitingTimes += waitingTimes[i];
    sumX2WaitingTimes += waitingTimes[i] * waitingTimes[i];

    sumXServiceTimes += serviceTimes[i];
    sumX2ServiceTimes += serviceTimes[i] * serviceTimes[i];
  }

  busyTime = sumXServiceTimes;

  avgInterArrivalTimes = sumXInterArrivalTimes / numCustomer;
  stdInterArrivalTimes = sqrt( (sumX2InterArrivalTimes - (avgInterArrivalTimes) * (avgInterArrivalTimes) * numCustomer) / (numCustomer - 1) );

  avgWaitingTimes = sumXWaitingTimes / numCustomer;
  stdWaitingTimes = sqrt( (sumX2WaitingTimes - (avgWaitingTimes) * (avgWaitingTimes) * numCustomer) / (numCustomer - 1) );

  avgServiceTimes = sumXServiceTimes / numCustomer;
  stdServiceTimes = sqrt( (sumX2ServiceTimes - (avgServiceTimes) * (avgServiceTimes) * numCustomer) / (numCustomer - 1) );

  // Print these values
  printf("%-20s%-20s%-20s\n", "", "Average Times", "Standard Deviations");
  printf("%-20s%-20s%-20s\n", "", "---------------", "---------------------");
  printf("%-20s%-20.2f%-20.2f\n", "Inter-Arrival", avgInterArrivalTimes, stdInterArrivalTimes);
  printf("%-20s%-20.2f%-20.2f\n", "Waiting", avgWaitingTimes, stdWaitingTimes);
  printf("%-20s%-20.2f%-20.2f\n", "Service", avgServiceTimes, stdServiceTimes);
}

// Function for calculating the mean and standard deviation of queue lengths
void onlineAvgStd2(){
  double sumX, sumX2;
  sumX = sumX2 = 0.0;

  int size = 0;
  numberOfElementsInQueue* n = nqHead;
  // traverse over the linked list
  while(n != NULL){
    sumX += n->numberOfElements;
    sumX2 += n->numberOfElements * n->numberOfElements;
    size = size + 1;
    n = n->next;
  }

  avgQueueLength = sumX / size;
  stdQueueLength = sqrt( (sumX2 - (avgQueueLength) * (avgQueueLength) * size) / (size - 1) );
  
  // Print these values
  printf("%-20s%-20.2f%-20.2f\n", "Queue length", avgQueueLength, stdQueueLength);
}


//----------------------------------------------------------------------------------------------------------------------------------------------------------------
// Create all the functions for queue operations
// Add for queue
bool enqueue(struct timeval arrivalTime){

  customer* cust = (customer *)malloc(sizeof(customer));
  if(cust == NULL) {
    fprintf(stderr, "Memory allocation error\n");
    return 0;
  }
  
  cust->arrivalTime = arrivalTime;
  cust->next = NULL;
  
  if(qLength == 0) {
    // This means queue is empty so insert the node. Point both qHead and qTail to cust
    qHead = cust;
    qTail = cust;
  } else {
    qTail->next = cust;
    qTail = cust;
  }
  qLength++;
  return 1;
}

// dequeue from queue
struct timeval* dequeue() {
  if(qLength == 0) {
    printf("Nothing to dequeue\n");
    return NULL;
  } else {
    
    customer* cust = qHead;
    qHead = qHead->next;
  
    // If there is only one element in the queue
    if(qHead == NULL) {
      qTail = NULL;
    }
    
    // Save the arrival time
    struct timeval* arrivalTime = (struct timeval*)malloc(sizeof(struct timeval));
    arrivalTime->tv_sec = cust->arrivalTime.tv_sec;
    arrivalTime->tv_usec = cust->arrivalTime.tv_usec;
    
    
    // Free the memory of the dequeued customer
    free(cust);
    
    qLength--;

    return arrivalTime;
  }
}

// Print Queue
void printQueue() {
  if(qLength == 0){
    printf("Queue is empty");
    fflush(stdout);
    printf("\033[2K\033[G");
  } else {
    customer* cust = qHead;
    int index = 0;
    printf("Queue : ");
    while(cust != NULL) {
      printf("%d", index);
      // printf("tvsec : %ld\n", cust->arrivalTime.tv_sec);
      cust = cust->next;
      index++;
    }

    if(lastPrint == false){
      printf("\033[2K\033[G");
    } else {
      printf("\n");
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------------
// Create the add function for addition of a node in the numberOfElementsInQueue list
bool enqueueNumberOfElementsInQueue(unsigned numberOfElementsAtATime){

  numberOfElementsInQueue* n = (numberOfElementsInQueue *)malloc(sizeof(numberOfElementsInQueue));
  if(n == NULL){
    fprintf(stderr, "Memory allocation error\n");
    return false;
  }

  n->next = NULL;
  n->numberOfElements = numberOfElementsAtATime;

  if(nqHead == NULL){
    nqHead = n;
    nqTail = n;
  } else {
    nqTail->next = n;
    nqTail = n;
  }
  return true;
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------------


void* thread1(){

  customer *newCustomer;

  while(lastCustomerFlag == false){
    struct drand48_data randData;
    struct timeval tv;
    double result;

    gettimeofday(&tv, NULL);

    srand48_r(tv.tv_sec + tv.tv_usec, &randData);
    drand48_r(&randData, &result);
    result = -log(1.0 - result) / lambda; //here result is the time we need to wait
    // This here is inter arrival time. 
    // Because the time we're arbitrarily sleeping is the time between two incoming processes.

    // Add this inter-arrival time to the array interArrivalTime at indexInterArrivalTime
    interArrivalTimes[indexInterArrivalTimes] = result;
    indexInterArrivalTimes++;


    struct timespec sleeptime; 
    sleeptime.tv_sec = (time_t)result;
    sleeptime.tv_nsec = (long)((result - (double)sleeptime.tv_sec) * 1e9);

    //sleep for sleeptime amount of time
    nanosleep(&sleeptime, NULL);
    newCustomer = (customer *)malloc(sizeof(customer));
    //gettimeofday() timestamp the arrival time
    gettimeofday(&(newCustomer->arrivalTime), NULL);

    
    //--------------------------------------------------
    //lock qMutex
    pthread_mutex_lock(&qMutex);
    //two cases
    //insert into a non empty queue
    if(qLength==0) {
      enqueue(newCustomer->arrivalTime);
      pthread_cond_signal(&condition);
    } else {
      enqueue(newCustomer->arrivalTime);
    }
    //unlock qMutex
    pthread_mutex_unlock(&qMutex);
    //--------------------------------------------------

  }

}

// Have to change the for loop to while loop. If the common index shared between the thread indexCustomerServing is equal to the last index i.e. indexServiceTimes == numCustomer-1 then break the while loop

void* thread2(){

  struct timeval* dequeuedTime;
  struct timeval tv;
  double waitingTime;

  while( 1 ){

    if( indexServiceTimes < numCustomer){
      //--------------------------------------------------
      //lock qMutex
      pthread_mutex_lock(&qMutex);
      if(qLength == 0) {
        //wait for signal
        pthread_cond_wait(&condition, &qMutex);
        dequeuedTime = dequeue();
      } else {
        //remove from head
        dequeuedTime = dequeue();
      }

      gettimeofday(&tv, NULL);
      waitingTime = tv.tv_sec - dequeuedTime->tv_sec + (tv.tv_usec - dequeuedTime->tv_usec)/1000000.0;
      // Here we've calculated the waiting time of this customer
      waitingTimes[indexWaitingTimes] = waitingTime;
      indexWaitingTimes++;
      
      //now draw service time from exponential distribution and sleep for that much time
      struct drand48_data randData;
      struct timeval tv1;
      double result;

      gettimeofday(&tv1, NULL);

      srand48_r(tv1.tv_sec + tv1.tv_usec, &randData);
      drand48_r(&randData, &result);
      result = -log(1.0 - result) / mu; 

      struct timespec worktime; // This timespec variable here represents service time
      // we draw service time from exponential distribution with the parameter mu
      worktime.tv_sec = (time_t)result;
      worktime.tv_nsec = (long)((result - (double)worktime.tv_sec) * 1e9);

      // convert this timespec into a double
      double serviceTime = (double)worktime.tv_sec + (double)worktime.tv_nsec / 1e9;
      // add this serviceTime to the array serviceTime at index indexServiceTime
      serviceTimes[indexServiceTimes] = serviceTime;
      indexServiceTimes++;

      //here i need to sleep for result time using nanosleep function. How would i do that?
      // This here doesnt need to be in a mutex
      nanosleep(&worktime, NULL);

      //here check if this is for the last iteration of the loop. If yes then change the lastCustomerFlag to true
      if(indexServiceTimes == numCustomer-1){
        lastCustomerFlag = true;
        printf("The number of customers served is %d\n", indexServiceTimes+1);
      }

      //unlock qMutex
      pthread_mutex_unlock(&qMutex);
      //--------------------------------------------------

    } else {
      break;
    }

  }
}

void *thread3(){
  while(lastCustomerFlag == false){
    // print the elements in the queue
    printQueue();

    // add the current number of elements in the queue to our special data structure
    enqueueNumberOfElementsInQueue(qLength);

    // sleep for 0.005 seconds
    struct timespec sleep5Seconds;
    sleep5Seconds.tv_sec = 0;
    sleep5Seconds.tv_nsec = 5000000;
    nanosleep(&sleep5Seconds, NULL);
  }

  lastPrint = true;
  printQueue();
  printf("Queue length thread has ended here\n");
}


int main(int argc, char *argv[]){

  time_t start, end;

  start = time(NULL);

  int option;
  while ((option = getopt(argc, argv, "l:m:c:s:")) != -1){
    switch(option){
      case 'l':
        lambda = atof(optarg);
        break;
      case 'm':
        mu = atof(optarg);
        break;
      case 'c':
        numCustomer = atoi(optarg);
        break;
      case 's':
        numServer = atoi(optarg);
        if(numServer<1 || numServer>5){
          fprintf(stderr, "Error: -s value should be between 1 and 5\n");
          return 1;
        }
        break;
      case '?':
        fprintf(stderr, "Unknown option : %c\n", optopt);
        return 1;
      default:
        fprintf(stderr, "Usage : %s -f <lambda> -m <mu>\n", argv[0]);
        return 1;
    }
  }

  if(lambda > mu * numServer) {
    fprintf(stderr, "Error: value of Lambda should be less than mu * numberOfServers\n");
    return 1;
  }

  interArrivalTimes = (double *)malloc(numCustomer * sizeof(double));
  waitingTimes = (double *)malloc(numCustomer * sizeof(double));
  serviceTimes = (double *)malloc(numCustomer * sizeof(double));

  if (interArrivalTimes == NULL || waitingTimes == NULL || serviceTimes == NULL) {
    fprintf(stderr, "Memory allocation error\n");
    return 1;
  }

  // initialize arrays with 0.0
  for (int i = 0; i < numCustomer; ++i) {
    interArrivalTimes[i] = 0.0;
    waitingTimes[i] = 0.0;
    serviceTimes[i] = 0.0;
  }

  pthread_t p1, p21, p22, p23 , p3;
  pthread_create(&p1, NULL, &thread1, NULL);
  pthread_create(&p21, NULL, &thread2, NULL);
  pthread_create(&p22, NULL, &thread2, NULL);
  pthread_create(&p23, NULL, &thread2, NULL);
  pthread_create(&p3, NULL, &thread3, NULL);
  pthread_join(p1, NULL);
  pthread_join(p21, NULL);
  pthread_join(p22, NULL);
  pthread_join(p23, NULL);
  pthread_join(p3, NULL);

  printf("5\n");
  onlineAvgStd1();
  onlineAvgStd2();
  printf("7\n");

  end = time(NULL);

  double totalTime = difftime(end, start);
  double serverUtilization = busyTime / totalTime * 100;
  // printf("Total Time: %.2f seconds\n", totalTime);
  // printf("Busy Time: %.2f seconds\n", busyTime);
  printf("------------ Server utilization : %f ------------\n", serverUtilization);


  // free(interArrivalTimes);
  // free(waitingTimes);
  // free(serviceTimes);

  return 0;
}