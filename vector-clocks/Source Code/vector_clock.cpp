#include <iostream>
#include <thread>
#include <chrono>
#include <ctime>
#include <fstream>
#include <random>
#include <mutex>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define BUFSIZE 1024
#define PORTNO 10000
const int MAXLINE=256;
/**
 * Author: Shamik Kundu
 * Date: 18/2/2017
 */
using namespace std;
using std::chrono::system_clock;
std::mutex mtx;
char *serverIP = "127.0.0.1";


class VectorClock{
    int internalEventNumber;
    int sendEventNumber;
    int receiveEventNumber;
    int *clock;
    int n;
    public:
        VectorClock(int n)
        {
            this->sendEventNumber = 0;
            this->internalEventNumber = 0;
            this->receiveEventNumber = 0;
            this->n = n;
            clock = new int[n];
            for (int i = 0; i < n; i++) {
                clock[i] = 0;
            }
        }

    void internalEvent(int processID) 
    {
        clock[processID] = clock[processID] + 1;
    }
    
    void messageSend(int processID)
    {

        clock[processID] = clock[processID] + 1;
    }

    void messageRecieve(int *clock_recv, int processID)
    {
        clock[processID] = clock[processID] + 1;

        for(int i = 0; i < n ; i++)
        {
            if(clock[i] < clock_recv[i])
            {
                clock[i] = clock_recv[i];
            }
        }
    }

    int* getClock(){
        return clock;
    }
    int  getSendNumber(){
        return sendEventNumber;
    }
    int getInternalNumber(){
        return internalEventNumber;
    }
    int getRecieveNumber(){
      return receiveEventNumber;
    }
    

};
/**
 * [InternalEventsManager: Takes care of internal events]
 * @param Test [Object of VectorClock Class which is private to each process]
 * @param processID [Process ID]
 * @param nofInternalEvents [number of internal events to be executed by the process]
 * @param lambda [delay]
 * @param nodes [number of processes]
 */
void InternalEventsManager(VectorClock &Test, int processID, int nofInternalEvents, int lambda, int nodes)
{
       std::default_random_engine generator;
       std::exponential_distribution<double> distribution (1.0/lambda);
       int eventNum = Test.getInternalNumber();
        
        for(int i = 1 ; i <= nofInternalEvents; i++)
    {                           
        mtx.lock();
        ofstream myfile;
        myfile.open ("log.txt", ios::app);
        myfile << "P" << processID<< " executes";        
        Test.internalEvent(processID); 
        double sleep_time = distribution (generator);       
        int *getVectorClock;
        getVectorClock = Test.getClock();
        time_t currentTime;
        struct tm *local;
        time(&currentTime);                   
        local = localtime(&currentTime);
        int hour   = local->tm_hour;
        int min    = local->tm_min;
        int sec    = local->tm_sec;  
        myfile <<" internal event e" << processID<< eventNum << " at " << hour << ":" << min <<":" << sec << ", vc: [";
        for(int i =0; i < nodes; i++)
        {
            myfile << getVectorClock[i] <<" ";
        }
        myfile << "]"<<endl;
        eventNum++;
        myfile.close();
        mtx.unlock();
        std::this_thread::sleep_for(std::chrono::duration<double> (sleep_time));
    }

}
/**
 * [SendEventsManager: Takes care of send events]
 * @param Test [Object of VectorClock Class which is private to each process]
 * @param processID [Process ID]
 * @param nofSendEvents [number of send events to be executed by the process]
 * @param lambda [delay]
 * @param topology[] [array containing the topology specific to the process denoted by processID ] 
 * @param size [size of the topology]
 * @param nodes [number of processes]
 */
void SendEventsManager(VectorClock &Test, int processID, int nofSendEvents, int lambda, int topology[], int size, int nodes)
{
        int sendNum = Test.getSendNumber();
        std::default_random_engine generator;
        std::exponential_distribution<double> distribution (1.0/lambda);
        for(int i = 1 ; i <= nofSendEvents; i++)
    {                       
        
        int RandIndex = rand() % size;
        int servPort = PORTNO + topology[RandIndex];
        
        mtx.lock();
        ofstream myfile;
        myfile.open ("log.txt", ios::app);
        myfile << "P" << processID<< " sends message "; 
        Test.messageSend(processID); 
        double sleep_time = distribution (generator);       
        int *getVectorClock;
        getVectorClock = Test.getClock();
        time_t currentTime;
        struct tm *local;
        time(&currentTime);

        //Create a socket
        int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sockfd < 0) {
            perror("socket() failed");
            exit(-1);
        }

        // Set the server address
        struct sockaddr_in servAddr;
        memset(&servAddr, 0, sizeof(servAddr));
        servAddr.sin_family = AF_INET;
        int err = inet_pton(AF_INET, serverIP, &servAddr.sin_addr.s_addr);
        if (err <= 0) {
            perror("inet_pton() failed");
            cout<<"inet_pton() failed";
            exit(-1);
        }
        servAddr.sin_port = htons(servPort);
        
        
        // Connect to server
        unsigned int microseconds = 4000000;
        while (connect(sockfd, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0) {
             perror("connect() failed");
             cout<<"connect() failed"; 
             usleep(microseconds);           
        }
        
        int *updatedVecotrClock = new int[nodes+2];
        int x;
        for(x = 0; x < nodes; x++){
           updatedVecotrClock[x] = getVectorClock[x];
        }
        updatedVecotrClock[nodes] = processID;
        updatedVecotrClock[nodes+1] = -1;
        int length = sizeof(int)*(nodes+2);
        printf("%d",length);
        // Send string to server
        int sentLen = send(sockfd, updatedVecotrClock, length, 0);
        if (sentLen < 0) {
            perror("send() failed");
            exit(-1);
        } else if (sentLen != length) {
            perror("send(): sent unexpected number of bytes");
            exit(-1);
        }   
        
        close(sockfd);
        
        local = localtime(&currentTime);
        int hour   = local->tm_hour;
        int min    = local->tm_min;
        int sec    = local->tm_sec;
       
        myfile <<"m" << processID<< sendNum << " to P" << topology[RandIndex] <<  " at " << hour << ":" << min <<":" << sec << ", vc: [";
        for(int i =0; i < nodes; i++)
        {
            myfile << getVectorClock[i] <<" ";
        }
        myfile << "]"<<endl;
        sendNum++;
        myfile.close();
        mtx.unlock();
        std::this_thread::sleep_for(std::chrono::duration<double> (sleep_time));
    }

}

/**
 *[RecvEventsManager : Takes care of the recieve events]
 * @param processID [Process ID]
 * @param nodes [number of processes]
 */

void RecvEventsManager(VectorClock &Test, int processID, int nodes)
{      
     int rcvNum = Test.getRecieveNumber();                
    int servSock, servPort = PORTNO + processID;
    if ((servSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        perror("socket() failed");
        exit(-1);
    }
    // Set local parameters
    struct sockaddr_in servAddr;
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port = htons(servPort); 

    // Bind to the local address
    if (bind(servSock, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0) {
        perror("bind() failed");
        exit(-1);
    }

    // Listen to the client
    if (listen(servSock, 10) < 0) {
        perror("listen() failed");
        exit(-1);
    }

    // Server Loop
    for (;;) {
        struct sockaddr_in clntAddr;
        socklen_t clntAddrLen = sizeof(clntAddr);

        // Wait for a client to connect
        int clntSock = 
                accept(servSock, (struct sockaddr *) &clntAddr, &clntAddrLen);
        if (clntSock < 0) {
            perror("accept() failed");
            exit(-1);
        }

        
        // Receive data
        int buffer[BUFSIZE];
        memset(buffer, 0, BUFSIZE);
        int recvLen = recv(clntSock, buffer, BUFSIZE - 1, 0);
        if (recvLen < 0) {
            perror("recv() failed");
            exit(-1);
        }
        int i = 0;
        while(buffer[i]!= -1){
            i = i+1;
        }
        int senderid = buffer[i-1];
    close(clntSock);  
    mtx.lock();
    ofstream myfile;
    myfile.open ("log.txt", ios::app);
    myfile << "P" << processID<< " receives";    
    Test.messageRecieve(buffer,processID);
    int *getVectorClock;
    getVectorClock = Test.getClock();
    time_t currentTime;
    struct tm *local;
    time(&currentTime);                   
    local = localtime(&currentTime);
    int hour   = local->tm_hour;
    int min    = local->tm_min;
    int sec    = local->tm_sec;
    myfile <<" m" << senderid<< rcvNum << " from P"<<senderid <<" at "  << hour << ":" << min <<":" << sec << ", vc: [";
    for(int i =0; i < nodes; i++)
    {
        myfile << getVectorClock[i] <<" ";
    }
    myfile << "]"<<endl;
    rcvNum ++;
    myfile.close();
    mtx.unlock();
}

}
/**[threadHandler: creates three threads per prcoess responsible for send, recieve and internal events.]
 * @param processID [Process ID]
 * @param local_port [Server port for the process]
 * @param internalEvent [total number of internal events to be executed by a specific process]
 * @param sendEvent [total number of send events to be executed by a specific process]
 * @param topology[] [array containing the topology specific to the process denoted by processID ] 
 * @param size [size of topology array] 
 * @param nodes [number of processes]
 */
int threadHandler(int processID, int local_port, int lambda, int internalEvent, int sendEvent, int topology[], int size, int nodes)
{
    std::thread t1, t2, t3; 
    std::thread handleRecieveEvent, handleInternalEvent, handleSendEvent; 
    VectorClock vectorClock(nodes);
    handleInternalEvent = std::thread(InternalEventsManager, std::ref(vectorClock), processID, internalEvent, lambda, nodes);
    handleRecieveEvent = std::thread(RecvEventsManager, std::ref(vectorClock), processID, nodes);
    handleSendEvent = std::thread(SendEventsManager, std::ref(vectorClock), processID, sendEvent, lambda, topology, size, nodes);
    handleRecieveEvent.join();
    handleInternalEvent.join();
    handleSendEvent.join();
    return 0;
}

/* [getTopology: extracts topology specific to a process from genereal topology]
 * @param nodes [total number of processes]
 * @param processid [Process ID]
 * @param topology[][] [the full adjacency list as read from input file]
 */

int* getTopology(int nodes, int processid, int topology[100][100]){
        int len = 0;
   for(int i = 1 ; i<nodes; i++){
       if (topology[processid][i]!=-1)
          len++;
       }
   int *topology_fixed = new int[len];
   int k =0;
   for(int i = 1 ; i<nodes; i++){
       if (topology[processid][i]!=-1){
          topology_fixed[k++] = topology[processid][i];
       }   
      }    
  return topology_fixed;
}

/* [getSize: get the size of topology array specific to each process]
 * @param processid [Process ID]
 * @param topology[][] [the full adjacency list as read from input file]
 */
int getSize(int nodes, int processid, int topology[100][100])
{
    int len = 0;
   for(int i = 1 ; i<nodes; i++){
       if (topology[processid][i]!=-1)
          len++;
       }
    return len;
}  
      
int main()
{
   int nodes=0, lambda, internal, pid , size , portno = PORTNO, send;
   ifstream inFile ("inp-params.txt");
   char oneline[MAXLINE];
   int topology[100][100];
   int linecount = 0;
   for (int i = 0 ; i<100;i++)
    for (int j =0;j<100;j++)
     topology[i][j]=-1;
   while (inFile)
   {
       inFile.getline(oneline, MAXLINE);
      // cout << oneline << endl;
       char * pch;
       pch = strtok (oneline," ");
      
     if(linecount==0)
     {   int f = 0;
         
         while (pch != NULL)
        {
         if (f == 0){
           nodes = atoi(pch);
          
      }
        if (f == 1){
           internal = atoi(pch);
      }
        if (f ==2){
           send = atoi(pch);
       }
       
       if (f ==3){
           lambda = atoi(pch);
       }
        pch = strtok (NULL, " /");
        f++;
        }
     }
     if (linecount!=0){
      int i = 0,j=0;
      int *a = new int[nodes];  
      for (int i = 0;i<nodes;i++)
        a[i]=-1;   
       while (pch != NULL)
      {
        a[i++] = atoi(pch);
        pch = strtok (NULL, " ");
       
      }
      for(int k = 0; k<nodes;k++){
        topology[linecount-1][k]=a[k];
      }
     } 
     linecount ++;
   }
  
   inFile.close();
   std::thread t[nodes];
   for(pid = 0; pid < nodes; pid++){
        int* top = getTopology(nodes,pid,topology);
        size = getSize(nodes,pid,topology);
        t[pid]= std::thread(threadHandler, pid, portno, lambda, internal, send, top, size, nodes);
        portno=portno+1;
    }

    for(pid = 0; pid < nodes; pid++)
        t[pid].join();

    return 0;
}
