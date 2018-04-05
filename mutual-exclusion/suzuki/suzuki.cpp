#include <vector>
#include <mutex>
#include <fstream>
#include <map>
#include <ctime>
#include <deque>
#include <stack>
#include <string>
#include <random>
#include <thread>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#define IDLE 0
#define INUSE 1
#define REQMSG 0
#define PRIVILEGE 1
#define True 1
#define False 0
#define PORTNO 50000
#define BUFSIZE 1024
long long int counter;

std::mutex mtx;
using namespace std;

class Node
{

public:
	vector<int> RN; // What is the latest seqNo information you have
	                          // about any other site
	vector<int> reqSites; // What all sites have requested from me
	// This is maintatined with token. LN[j] means the seqNo of site j which
	// it has latest executed.
	// Ex. LN[3] contains the information of the latest seqNo execution made
	// by Site 3.
	vector<int> LN;
	int nodeId; // Process Id of the Node
	int seqNo; // This is my say 10th access to CS. Then seqNo=10
	int clocks;
	int hasToken; // 0 - Does not have Token, 1 - Has Token
	bool requesting;
	int delay1;
	int delay2;
	int maxAccess; // Total how many access you can have to CS
	int noFnodes; // No. of nodes inside the system
    map<int,string> v;
    vector<int> neighbours;
	Node()
	{
		seqNo = 0;
		clocks = 0;
		delay1 = 0;
		delay2 = 0;
		maxAccess = 0;
		noFnodes = 0;
		requesting = false;
	}

	void initRNLN(int noFnodes)
	{
		for (int i = 0; i < noFnodes; i++)
		{
			RN.push_back(-1);
			LN.push_back(-1);
		}
	}

	void setseqNo(int _seqNo)
	{
		seqNo = _seqNo;
	}
	int getseqNo()
	{
		return seqNo;
	}
	void setClock(int _clock){
      clocks = _clock;
    }
    void incClock()
    {
    	clocks = clocks + 1;
    }
    int getClock(){
      return clocks;
    }
    void sethasToken(int _hasToken)
    {
    	hasToken = _hasToken;
    }
    int gethasToken()
    {
    	return hasToken;
    }
};

void testSend(Node &Test, int processID, int msgType, int value);

int max(int a, int b){
  if (a > b)
      return a;
  else
      return b;
}

// It is used to get the system current time stamp.

long long int getTime()
{
  // get time at that instant
  struct timeval time1;
  gettimeofday(&time1, NULL);
  return time1.tv_sec*1000000 + time1.tv_usec;
}

void time()
{
  time_t currentTime;
  struct tm *local;
  long long int microSec;

  time(&currentTime);
  local = localtime(&currentTime);

  int Hour   = local->tm_hour;
  int Min    = local->tm_min;
  int Sec    = local->tm_sec;

  long long int currTimeStamp = getTime();

  /**
   * microseconds will be getting printed
   */
  microSec = currTimeStamp % 100000;
  // print the current time
  std::cout << Hour << ":" << Min << ":" << Sec << ":" << microSec;
  //std::cout<< microSec << endl;

}

void request_cs(Node &Test)
{
  long long int requesttime;
	Test.requesting = true;
	if (Test.gethasToken() == False)
	{
    mtx.lock();
		Test.RN[Test.nodeId] = Test.RN[Test.nodeId] + 1; // Increment my own SeqNo
		int val = Test.RN[Test.nodeId];
    mtx.unlock();

		// Request the CS
		cout << endl<< endl<< Test.seqNo+1 << "th CS Request at ";
		time();
		cout <<" by Process " << Test.nodeId << endl<<endl;

		for (int i = 0; i < Test.noFnodes; i++)
		{
			// Broadcast your CS entry will to all your childrens along with
			// the details of your seqNo. i.e. say I am requesting 10th time
			if (Test.nodeId == i)
				continue;
      requesttime = getTime();
			testSend(Test,i,REQMSG, val);
		}
		while(Test.gethasToken() == False){}
	}
    long long int getresponse = getTime();
    counter = counter + (getresponse - requesttime);
	// Enter the CS
    cout <<endl<<endl<< Test.seqNo+1 << "th CS Entry at ";
    //mtx.unlock();
      
	time();
	cout << " by Process " << Test.nodeId << endl<<endl;
	std::this_thread::sleep_for(std::chrono::duration<double> (Test.delay1));

	// Releasing the CS
	mtx.lock();
	Test.LN[Test.nodeId] = Test.RN[Test.nodeId];// My latest request for CS is fullfilled
	mtx.unlock();

	cout <<endl<<endl<< Test.seqNo+1 << "th CS Exit at " <<endl;;
  cout << "Average Response time till "<< Test.seqNo+1 << "access is :" << float(counter)/Test.maxAccess;
	Test.seqNo = Test.seqNo + 1;

	time();
	cout << " by Process " << Test.nodeId << endl<<endl;
  //cout << "Average Response time till "<< Test.seqNo+1 << "access is :" << float(counter)/Test.maxAccess;
	//std::this_thread::sleep_for(std::chrono::duration<double> (Test.delay2));

	for (int i = 0; i < Test.noFnodes; i++)
	{
		if(Test.nodeId == i) // Note the processes will have id's starting from 0
			continue;
		if(Test.RN[i] == Test.LN[i] +1)
		{
			if (std::find(Test.reqSites.begin(), Test.reqSites.end(), i) != Test.reqSites.end())
        mtx.lock();
				Test.reqSites.push_back(i);
        mtx.unlock();
		}
	}

	if (!Test.reqSites.empty()) // Queue is not empty
	{
		int reqNodeId = Test.reqSites.front();
    mtx.lock();
		Test.reqSites.erase(Test.reqSites.begin());
		Test.sethasToken(False);
    mtx.unlock();
		testSend(Test,reqNodeId, PRIVILEGE, 0); // 0 some bad value
	}
	mtx.lock();
	Test.requesting = false;
	mtx.unlock();
}

void recv_request(Node &Test, int *buffer)//int reqNodeId, int val)
{
	//cout << Test.nodeId << " " << "received CS request from " << reqNodeId << endl;
	int reqNodeId = buffer[1];
	int val = buffer[2];
	int clockValue = buffer[3];
	int updatedClock = max(Test.getClock(), clockValue);
	Test.setClock(updatedClock);
	int updateSeq = max(Test.RN[reqNodeId], val);
  mtx.lock();
	Test.RN[reqNodeId] = updateSeq;
  mtx.unlock();
	// Test.RN[reqNodeId] == LN[reqNodeId]+1
	// LN[i] represents that ith site kitni baar CS ko execute kar chuki hai
	// Suppose 9 baar kar chuki hai aur ye request 10th hai i.e.
	// Test.RN[reqNodeId] = 10; yani ith ek baar aur CS jana chahati hai. then
	// 9+1 = 10 ; to mai simply token bhej doonga
	if ((Test.gethasToken() == True ) && (!Test.requesting) && (Test.RN[reqNodeId] == Test.LN[reqNodeId]+1))
	{
		// Send the token to the requester
    mtx.lock(); 
		Test.sethasToken(False);
    mtx.unlock();
		//Token token;
		//queueSize = token.deque.size();
		testSend(Test,reqNodeId, PRIVILEGE, 0);
		// testSend(Test,Test.neighbours[reqNodeId],Test.nodeId, val);
	}
}

// void tokenRecv(Node &Test, int *buffer)
// {


// 	cout << Test.nodeId << " " << "received token from " << "senderId" << endl;
// 	Test.sethasToken(True);
// }


void testRecvEvents(Node &Test){
    // Run your server socket connection, for infinite amount of time to keep accepting the connections
	//cout << "Server Code is running " << endl;
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
        if (buffer[0] == PRIVILEGE){
            // mtx.lock();
            // ofstream myfile;
            // myfile.open ("log.txt", ios::app);
            time();
            cout <<" Node " << Test.nodeId << " received Token  " << "From Node " << buffer[1] << endl;
            mtx.lock();
            Test.hasToken = True;
              int i;
              for (i = 0; i < Test.noFnodes; i++)
              {
                Test.LN[i] = buffer[i+2]; // 0 contains MsgType, 1 contains Nodeid
              }
            mtx.unlock();

            // myfile.close();
            // t3 = std::thread(tokenRecv, std::ref(Test), std::ref(buffer));
            // t3.join();
        }
        if (buffer[0] == REQMSG){
            // mtx.lock();
            // ofstream myfile;
            // myfile.open ("log.txt", ios::app);
            time();
            cout << " Node " << Test.nodeId << " received CS Request Message From "<< buffer[1] << endl;
            // mtx.unlock();
            t4 = std::thread(recv_request, std::ref(Test), std::ref(buffer));
            t4.join();
        }
        close(clntSock);
    }
}

void testSend(Node &Test, int processID, int msgType, int val){

  int elements, *buffer ;
  // Create Message
  if (msgType == REQMSG){

    // Request Message
    	buffer = new int[5], elements = 5;
        buffer[0] = REQMSG;
        buffer[1] = Test.nodeId;
        buffer[2] = val;
        buffer[3] = Test.getClock();
        buffer[4] = -1;
        mtx.lock();
        // Update Clock
        Test.setClock(Test.getClock() + 1);

        // Write to File
        //ofstream myfile;
        //myfile.open ("log.txt", ios::app);
        time();
        cout << " Node " << Test.nodeId << " send a CS request message to Node " << processID << endl;
        //myfile.close();
        mtx.unlock();
  }
  else if(msgType == PRIVILEGE){

  		int queueSize = Test.reqSites.size();
  		int numOfnodes = Test.LN.size();
  		elements = numOfnodes + queueSize + 3;
  	    buffer = new int[elements];
  	    int i, j = 2;
      // Control Message
        buffer[0] = PRIVILEGE;
        buffer[1] = Test.nodeId;
        buffer[2] = -1; // Filling invalid value
        for(i = 0; i < Test.LN.size(); i++)
        {
        	buffer[j] = Test.LN[i];
        	j++;
        }
        for(i = 0; i < Test.reqSites.size(); i++)
        {
        	buffer[j] = Test.reqSites[i];
        	j++;
        }
        buffer[j] = -1;
        // Write to File
        //ofstream myfile;
        //myfile.open ("log.txt", ios::app);
        time();
        cout << " Node " << Test.nodeId << " send token to Node " << processID << endl;
       // myfile.close();
}

  int servPort = PORTNO;
  string serv_IP = Test.v[processID];
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

int main(int argc, char const *argv[])
{
	if (argc != 3) {
        cout << "<Process ID> <hasToken>";
        exit(-1);
    }

  int yourId = atoi(argv[1]);
  int delay1, delay2, n, linenumber = 0;
  Node node;
  node.nodeId = yourId;
  node.hasToken = atoi(argv[2]);
  node.seqNo = 0;
  node.clocks = 0;

//read the input from the file
    string inFileNameParameters = "inp_suz.txt";

    fstream inFileParameters;

    inFileParameters.open(inFileNameParameters, ios::in);

    if(!inFileParameters.is_open()){
        cout << "Cannot open File" << inFileNameParameters;
        return -1;
    }
    while(!inFileParameters.eof()){

        if (linenumber == 0){
            inFileParameters >> n;
            inFileParameters >> delay1;
            inFileParameters >> delay2;
            inFileParameters >> ws;
            linenumber++;
            continue;
        }
        else if(linenumber <=n){
            string pids, line;
            int maxaccess, pid = 0;

            inFileParameters >> pid;
            getline(inFileParameters, pids, '-');
            getline(inFileParameters, line, ':');
            inFileParameters >> maxaccess;
            inFileParameters >> ws;
            node.v.insert(pair<int, string>(pid,line));
            if(pid == yourId){
                node.maxAccess = maxaccess;
            }
            //i++;
            linenumber++;
        }
        else{
            int pid = 0, neighbours = 0;
            std::string input, dummyData;
            inFileParameters >> pid;
            if (pid == yourId){
                getline(inFileParameters, input);
                stringstream stream(input);
                while(1) {
                   int n;
                   stream >> n;
                   if(!stream)
                      break;
                   node.neighbours.push_back(n);
                   neighbours++;
                }
                // node.setnoFNeighbours(neighbours);
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

    node.initRNLN(node.noFnodes);

    inFileParameters.close();

    cout << "node.pid" << " " << node.nodeId << endl;
    cout << "node.maxAccess" << " " << node.maxAccess << endl;
    // cout << "node.delay1" << " " << node.delay1 << endl;
    // cout << "node.delay2" << " " << node.delay2 << endl;
    // cout << "node.noFnodes"<< " " << node.noFnodes << endl;
    // cout << "node.hasToken" <<" " << node.hasToken << endl;
    // cout << "no.neighbours" <<" " << node.neighbours.size() << endl;
    // for (int i = 0; i < node.neighbours.size(); i++)
    // {
    // 	cout << node.neighbours[i] << endl;
    // }
    // cout << "map " << node.v[0] << endl;
    // cout << "map " << node.v[1] << endl;
    // cout << "map " << node.v[2] << endl;

    //  for (int i = 0; i < node.RN.size(); i++)
    // {
    // 	cout << node.RN[i] << endl;
    // }

    // Create the Threads
    std::thread t1, t2;

    t1 = std::thread(testRecvEvents, std::ref(node));
    std::this_thread::sleep_for(std::chrono::seconds(3));

    //  // mtx.lock();
    //   ofstream myfile;
    //   myfile.open ("log.txt", ios::app);
    //   cout << "Node " << yourId << " is initially Active" << endl;
    //   myfile.close();
    //   //mtx.unlock();
    for (int i = 0; i < node.maxAccess; i++)
   {
   	 request_cs(std::ref(node));
     std::this_thread::sleep_for(std::chrono::duration<double> (node.delay2));
       //if(i == node.maxAccess-1)
       // cout << "Average Response time is :" << float(counter)/node.maxAccess;
   }
    
    t1.join();
   return 0;
}
