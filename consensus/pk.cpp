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
#define PORTNO 70000
#define BUFSIZE 1024
#define DEFAULT 0
#define MSG_PREF 0
#define MSG_TB 1
/*
* Author: Shamik Kundu
* Roll number: CS16MTECH11015
*/
using namespace std;
std::mutex mtx;

class Node
{
public:
	int pid; //Process ID
	int n; // Total number of process
	int pref; //stores the prefrence value according to algorithm
	int maj; // stores the majority value as per the alogrithm
	int mult; // stores the mult value as per the algorithm
	int f; // number of faulty processes
	int isFaulty; // whether a process is faulty or not (supplied in run time)
	int *arr; // stores the PREF values sent by other processes
	int zeroes; // stores the count of zeroes in the array arr
	int ones; // stores the count of ones in the array ones
	int tb_val; // stores the TIEBREAKER value sent by phase king
  int arr_pos; // array index indicator 
	vector<int> prefvalues;
    map<int,string> v; // stores the process IDs and their corresponding IP addresses
  Node(int nn)
  {
  	pid = 0;
  	n = nn;
  	pref = 0;
  	maj = -1;
    arr_pos = 0;
  	mult = 0;
  	zeroes = 0;
  	ones = 0;
  	f = 0;
  	tb_val = -1111;
  	arr = new int[n-1];
  	for (int i = 0; i < n-1; ++i)
  	{
  		arr[i] = -1111;
  	}
  }
};

/**
 * @brief       Sends message 
 *
 * @param      Test        The test js a object of class Node
 * @param[in]  receiverID  The receiver id is the id to whoom the message should be sent
 * @param[in]  msgType     The message type can be either PREF or TIEBREAKER
 */
void testSend(Node &Test, int receiverID, int msgType){

  int *buffer = new int[4], elements = 4;

  // Create Message
  if (msgType == MSG_PREF){
       if(Test.isFaulty==0){
        srand(time(NULL));
        int sendPref = rand() % 2;
        buffer[0] = MSG_PREF;
        buffer[1] = sendPref;
        buffer[2] = Test.pid;
        buffer[3] = -1;}
        else{
        buffer[0] = MSG_PREF;
        buffer[1] = Test.pref;
        buffer[2] = Test.pid;
        buffer[3] = -1;
        }
  }
  else if(msgType == MSG_TB){
      if(Test.isFaulty==0){
      	srand(time(NULL));
        int sendPref = rand() % 2;
        buffer[0] = MSG_TB;
        buffer[1] = sendPref;
        buffer[2] = Test.pid;
        buffer[3] = -1;}
      else{
      	buffer[0] = MSG_TB;
        buffer[1] = Test.pref;
        buffer[2] = Test.pid;
        buffer[3] = -1;
      }
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
 * @brief      Recieves message 
 *
 * @param      Test       The test is a object of class Node
 * @param[in]  processID  The process id is the id of the process to whoom the message is sent
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
        	//  cout << buffer[i] << endl;
            i = i+1;
        }
        
        if (buffer[0] == MSG_PREF){
            mtx.lock();
            Test.arr[Test.arr_pos++] = buffer[1];
            cout<<"recieved PREF from process" << buffer[2]<<endl;
            cout<<"pref size"<<Test.arr_pos<<endl;
            mtx.unlock();
        }
        if (buffer[0] == MSG_TB){
        	mtx.lock();
        	Test.tb_val = buffer[1];
          cout<<"recieved TB from process" << buffer[2]<<endl;
        	//cout << "Test.J " << Test.J << endl;
        	mtx.unlock();
        }

        close(clntSock);
              
    }
}

/**
 * @brief       Round 1 according to the algorithm
 *
 * @param      Test  The test is a object of class Node
 */
void Round1(Node &Test){
	
	
  	for (int i = 1; i <= Test.n; ++i)
  	{
  		if(Test.pid!=i){
             
             testSend(Test,i,MSG_PREF);
             cout<< "sending PREF to " << i <<endl;
  		}
  	}
    cout<< "waiting for PREFS"<<endl;
	while(Test.arr[Test.n-2]==-1111){};
    cout<<"done getting all the PREFS"<<endl;
  	int zeroes, ones;
  	for (int i = 0; i < Test.n -1; ++i)
  	{
  		if(Test.arr[i] ==0){
  			mtx.lock();
  			 Test.zeroes++;
  			mtx.unlock();
  		}
  		else if(Test.arr[i] == 1){
  			mtx.lock();
  			Test.ones++;
  			mtx.unlock();
  		}
  	}
  	if(Test.zeroes>(Test.n/2)){
  		mtx.lock();
  		Test.maj = 0;
  		Test.mult = Test.zeroes;
        mtx.unlock();
    }
    else if(Test.ones>(Test.n/2)){
    	mtx.lock();
  		Test.maj = 1;
  		Test.mult = Test.ones;
        mtx.unlock();
    }
    else{
    	mtx.lock();
    	Test.maj = DEFAULT;
    	mtx.unlock();
    }
    mtx.lock();
  for (int i = 0; i < Test.n-1; ++i)
  {
    Test.arr[i] = -1111;
  }
  Test.zeroes = 0;
  Test.ones = 0;
  Test.arr_pos = 0;
  mtx.unlock();
    cout<<"end of Round1"<<endl;

}

/**
 * @brief      Round 2 as per the algorithm
 *
 * @param      Test      The test is a object of class Node
 * @param[in]  isKing    Indicates if the process is a king
 * @param[in]  numphase  The numphase is current phase number
 */
void Round2(Node &Test,int isKing,int numphase){
 if(isKing ==1){
 	for (int i = 1; i <= Test.n; ++i)
 	{
 		if(Test.pid!=i){
 			testSend(Test,i,MSG_TB);
      cout<<"phase king---sending TB to "<<i<<endl;
 		}
 	}
  cout<< "At the end of phase "<<numphase<<", the decision by process "<<Test.pid<<" is= "<<Test.pref<<endl;
 }
 else{
 	while(Test.tb_val==-1111){};
 	if(Test.mult>((Test.n/2)+Test.f)){
 		mtx.lock();
 		Test.pref = Test.maj;
 		cout<< "At the end of phase "<<numphase<<", the decision by process "<<Test.pid<<" is= "<<Test.pref<<endl;
 		mtx.unlock();
 	}
 	else{
 		mtx.lock();
 		Test.pref = Test.tb_val;
 		cout<< "At the end of phase "<<numphase<<", the decision by process "<<Test.pid<<" is= "<<Test.pref<<endl;
 		mtx.unlock();
 	}
 }
 mtx.lock();
  Test.tb_val = -1111;
  mtx.unlock();
}

int main(int argc, char const *argv[]){
    if (argc != 4) {
        cout << "<Process ID> <IsFaulty> <TotalNumProcess>";
        exit(-1);
    }

    
    int myid = atoi(argv[1]);
    int flt = atoi(argv[2]);
    int nmp = atoi(argv[3]);
    Node node(nmp);
    node.pid = myid;
    node.isFaulty = flt;

    string inFileNameParameters = "inps.txt";
    fstream inFileParameters;
    inFileParameters.open(inFileNameParameters, ios::in);
    if(!inFileParameters.is_open()){
        cout << "Cannot open File" << inFileNameParameters;
        return -1;
    }
    int numprocess=0, faultyprocess, linenumber = 0;
    // inFileParameters >> numprocess >> faultyprocess;
    // cout << numprocess << "   " << faultyprocess<<endl;
    while(!inFileParameters.eof()){

        if (linenumber == 0){
            inFileParameters >> numprocess;
            inFileParameters >> faultyprocess;
            inFileParameters >> ws;
            linenumber++;
            continue;
        }
        else if(linenumber <=numprocess){
            string pids, line;
            int prf, pid = 0;
            inFileParameters >> pid;
            getline(inFileParameters, pids, '-');
            getline(inFileParameters, line, ':');
            inFileParameters >> prf;
            inFileParameters >> ws;
            node.v.insert(pair<int, string>(pid,line));
            if(pid == myid){
                node.pref = prf;
            }
            //i++;
            linenumber++;
        }
    }

    node.f = faultyprocess;

   std::thread t1;
   t1 = std::thread(testRecvEvents, std::ref(node), myid);
   std::this_thread::sleep_for(std::chrono::seconds(3));
   for (int k = 1; k <= faultyprocess+1; ++k)
   {
   	   Round1(std::ref(node));
   	   if(node.pid==k)
   	   Round2(std::ref(node),1,k);
   	   else
   	   Round2(std::ref(node),0,k);
   }

   t1.join();
	return 0;
}


