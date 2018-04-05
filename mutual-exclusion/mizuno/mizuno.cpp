/**

*Neilsen-Mizuno distributed mutual exclusion algorithm implementation

*Compilation : g++ -std=c++11 -pthread mizuno.cpp -o mu
*Execution   : ./mu > output.txt

*This will generate the output in the file named as output.txt
*We need to collect the output files generated in the various VM's in order to
 verify the correctness of the algorithm.

*Reference : "A Dag-Based Algorithm for Distributed Mutual Exclusion" paper by
			  Neilsen-Mizuno

*/
#include <iostream>
#include <vector>
#include <mutex>
#include <fstream>
#include <iostream>
#include <typeinfo>
#include <sstream>
#include <string>
#include <random>
#include <ctime>
#include <thread>
#include <map>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>

// Msg Types: REQUEST, PRIVILAGE, INITIAL
#define REQUEST 0
#define PRIVILAGE 1
#define INITIAL 2

#define PORTNO 70000 // Fix the port
#define BUFSIZE 1024
long long int counter;
using namespace std;
std::mutex mtx;

/**
 * @brief      Template for specifying the attributes of a Process inside a
 *             Distributed System
 */
class Node
{
public:
	int pid;
	int accessNo; // This is my say 10th access to CS. Then seqNo=10
	int delay1;
	int delay2;
	bool HOLDING;
	int LAST;
	int NEXT;
	int isInit;
	int J;
	int noFnodes;
	int maxAccess; // Total how many access you can have to CS
	vector<int> neighbours;
    map<int,string> v;
  Node()
  {
  	accessNo = 0; // This is my say 10th access to CS. Then seqNo=10
	noFnodes = 0;
	HOLDING = false;
	delay1 = 0;
  
	delay2 = 0;
	LAST = -1;
	NEXT = -1;
	J = -9999;
	isInit = -9999;
	maxAccess = 0; // Total how many access you can have to CS
  }
};
void testSend(Node &Test,int,int,int,int);

/**
 * @brief      Gets the system current timestamp.
 *
 * @return     System time.
 */
long long int getTime()
{
  // get time at that instant
  struct timeval time1;
  gettimeofday(&time1, NULL);
  return time1.tv_sec*1000000 + time1.tv_usec;
}

// It outputs the current time of the system
void time()
{
    time_t currentTime;
    struct tm* local;
    long long int microSec;

    time(&currentTime);
    local = localtime(&currentTime);

    int Hour = local->tm_hour;
    int Min = local->tm_min;
    int Sec = local->tm_sec;
    long long int currTimeStamp = getTime();

  /**
   * microseconds will be getting printed
   */
	microSec = currTimeStamp % 10000;
	// print the current time
	std::cout << Hour << ":" << Min << ":" << Sec << ":" << microSec;
	//std::cout<< microSec << endl;
}

/**
 * @brief      Request for Critical Section
 *
 * @param      Test  It is the processId structure
 */
void P1(Node &Test)
{
  long long int requesttime;
	if (!Test.HOLDING) // if you don't hold the token, then send token request.
	{
		// Request for CS
		cout << Test.accessNo+1 << "th CS Request at ";
    requesttime = getTime();
		testSend(Test, Test.LAST, 0, Test.pid, Test.pid);
		time();
		cout <<" by Process " << Test.pid << endl;
		mtx.lock();
		Test.LAST = 0;
		mtx.unlock();
		while(!Test.HOLDING){}
	}
    mtx.lock();
	Test.HOLDING = false;

    // Enter the CS
    cout << Test.accessNo+1 << "th CS Entry at ";
    mtx.unlock();
    long long int getresponse = getTime();
    counter = counter + (getresponse - requesttime);
	time();
	cout << " by Process " << Test.pid << endl;
	std::this_thread::sleep_for(std::chrono::duration<double> (Test.delay1));

	// Exit the CS
	cout << Test.accessNo+1 << "th CS Exit at ";
	Test.accessNo = Test.accessNo + 1;

	time();
	cout << " by Process " << Test.pid << endl;
  cout << "Average Response time till "<< Test.accessNo+1 << "access is :" << float(counter)/Test.maxAccess<<endl;

	if (Test.NEXT != 0) // Someone wants the cs next
	{
		testSend(Test,Test.NEXT, 1, 0, 0);
		mtx.lock();
		Test.NEXT = 0;
		mtx.unlock();
	}
	else
	{
		mtx.lock();
		Test.HOLDING = true;
		mtx.unlock();
	}
}

/**
 * @brief      { function_description }
 *
 * @param      Test  It is the processId structure
 * @param[in]  source      Source is the neighbouring node which send the CS request
 * @param[in]  originator  Who originated the CS request message
 */
void P2(Node &Test, int source, int originator)
{

	if (Test.LAST == 0) // if last in the queue
	{
		if (Test.HOLDING) // has the token and not in CS
		{
			testSend(Test,originator,1,0,0);
     		mtx.lock();
			Test.HOLDING = false;
        	mtx.unlock();
		}
		else
		{
      		mtx.lock();
			Test.NEXT = originator;
      		mtx.unlock();
		}
	}
	else
	{

		testSend(Test,Test.LAST,0,Test.pid,originator); // forward request
    
	}
  	mtx.lock();
	Test.LAST = source;
  	mtx.unlock();
}

/**
 * @brief      Initialize the Code for Mutual Exclusion Algorithm Execution
 *
 * @param      Test  It is the processId structure
 */
void INIT(Node &Test)
{
	if (Test.HOLDING)
	{
		Test.HOLDING = true;
		Test.LAST = 0;
		Test.NEXT = 0;

		for (int i = 0; i < Test.neighbours.size(); ++i)
		{
			testSend(Test,Test.neighbours.at(i),2,Test.pid,0);
		}
	}
  	else
  	{
  		while(Test.J==-9999) {}
	    mtx.lock();
	    Test.HOLDING = false;
	    Test.LAST = Test.J;
	    Test.NEXT = 0;
	    cout << "Test.next " << Test.NEXT << endl;
	    cout << "Test.last " << Test.LAST << endl;

	    mtx.unlock();
	    for (int i = 0; i < Test.neighbours.size(); ++i)
	    {
	    	if(Test.neighbours.at(i)!= Test.J)
       			testSend(Test,Test.neighbours.at(i),2,Test.pid,0);
	    }
  	}
}

/**
 * @brief      Server Code run by the Process
 *
 * @param      Test  It is the processId structure
 * @param[in]  processID  ProcessId of the Node
 */
void testRecvEvents(Node &Test, int processID){

    int servSock, servPort = PORTNO;
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
    if (listen(servSock, 20) < 0) {
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

        int *buffer = new int[BUFSIZE];
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
        std::thread t3, t4;
        if (buffer[0] == 0){
         int originator = buffer[2];
	       int source = buffer[1];
	       t4 = std::thread(P2, std::ref(Test), source, originator);
           t4.join();
        }
        if (buffer[0] == 1){
            mtx.lock();
            Test.HOLDING = true;
            mtx.unlock();
        }
        if (buffer[0] == 2){
        	mtx.lock();
        	Test.J = buffer[1];

        	mtx.unlock();
        	t3 = std::thread(INIT, std::ref(Test));
            t3.join();
        }
        close(clntSock);
    }
}

/**
 * @brief      Send the message over Sockets using TCP
 * @param      Test  It is the processId structure
 * @param[in]  receiverID  Id to which the message will be send
 * @param[in]  msgType     REQUEST/PRIVILAGE/INITIAL
 * @param[in]  subType1    Used to serve multiple purpose
 * @param[in]  subType2    Used to serve multiple purpose
 */
void testSend(Node &Test, int receiverID, int msgType, int subType1, int subType2){

  int *buffer = new int[4], elements = 4;

  // Create Message
  if (msgType == REQUEST){

    // Request Message
        buffer[0] = REQUEST;
        buffer[1] = subType1;
        buffer[2] = subType2;
        buffer[3] = -1;
  }
  else if(msgType == PRIVILAGE){
      // Privilage Message
        buffer[0] = PRIVILAGE;
        buffer[1] = 0;
        buffer[2] = 0;
        buffer[3] = -1;
  }
  else if(msgType == INITIAL){
  	// Initialization Message
  	buffer[0] = INITIAL;
  	buffer[1] = Test.pid;
  	buffer[2] = -1;
    buffer[3] = -1;
  }

  int servPort = PORTNO;
  string serv_IP = Test.v[receiverID];
  const char * servIP = serv_IP.c_str(); // Convert ip to char const

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
  int err = inet_pton(AF_INET, servIP, &servAddr.sin_addr.s_addr);
  if (err <= 0) {
      perror("inet_pton() failed");
      exit(-1);
  }
  servAddr.sin_port = htons(servPort);

  // Connect to server
  while(connect(sockfd, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0) {
      perror("connect() failed");
      std::this_thread::sleep_for(std::chrono::seconds(5));
  }
  //cout << "connect success";
  int length = sizeof(int) *(elements);
  int sentLen = send(sockfd, buffer, length, 0);
  if (sentLen < 0) {
      perror("send() failed");
      exit(-1);
  } else if (sentLen != length) {
      perror("send(): sent unexpected number of bytes");
      exit(-1);
  }

  // Close the socket after the send is finished.
  close(sockfd);
}
/**
 * @brief      Main Function, starts the whole application
 *
 * @param[in]  argc  Number of commandline arguments it needs
 * @param      argv  Array containing the command-line argument values
 *
 * @return     if return=0, Successful Execution
 */
int main(int argc, char const *argv[])
{
	if (argc != 3)
	{
		cout << "<Process ID> <IsInitiator>";
		exit(-1);
    }
    Node node;
  	int yourId = atoi(argv[1]);
  	node.pid = yourId;
  	node.isInit = atoi(argv[2]);
  	//node.maxAccess = atoi(argv[3]);
  	int delay1, delay2, n, linenumber = 0;

	//read the input from the file
    string inFileNameParameters = "inp.txt";
    fstream inFileParameters;
    inFileParameters.open(inFileNameParameters, ios::in);

    if(!inFileParameters.is_open())
    {
    	cout << "Cannot open File" << inFileNameParameters;
        return -1;
    }
    while(!inFileParameters.eof())
    {
    	if (linenumber == 0)
    	{
    		inFileParameters >> n;
            inFileParameters >> delay1;
            inFileParameters >> delay2;
            inFileParameters >> ws;
            linenumber++;
            continue;
        }
        else if(linenumber <=n)
        {
            string pids, line;
            int maxaccess, pid = 0;
            inFileParameters >> pid;
            getline(inFileParameters, pids, '-');
            getline(inFileParameters, line, ':');
            inFileParameters >> maxaccess;
            inFileParameters >> ws;
            node.v.insert(pair<int, string>(pid,line));
            if(pid == yourId)
            {
            	node.maxAccess = maxaccess;
            }
            linenumber++;
        }
        else
        {
        	int pid = 0, neighbours = 0;
            std::string input, dummyData;
            inFileParameters >> pid;
            if (pid == yourId)
            {
            	getline(inFileParameters, input);
                stringstream stream(input);
                while(1)
                {
                	int n;
                    stream >> n;
                    if(!stream)
                    	break;
                    node.neighbours.push_back(n);
                    neighbours++;
                }
            }
            getline(inFileParameters,dummyData);
        }

        if(!inFileParameters)
        {
        	break;
        }
    }

    node.delay1 = delay1;
    node.delay2 = delay2;
    node.noFnodes = n;
    std::thread t2;
    if (node.isInit == 1){
  	 node.HOLDING = true;
     INIT(std::ref(node));
  }

   std::thread t1;
   t1 = std::thread(testRecvEvents, std::ref(node), yourId);
   std::this_thread::sleep_for(std::chrono::seconds(3));
   for (int i = 0; i < node.maxAccess; i++)
   {
   	 P1(std::ref(node));
     std::this_thread::sleep_for(std::chrono::duration<double> (node.delay2));
   }

   t1.join();
   return 0;
}
