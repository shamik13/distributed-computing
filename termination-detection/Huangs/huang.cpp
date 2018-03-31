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
#include <netinet/in.h>
#define BUFSIZE 1024
#define PORTNO 50000
#define ACTIVE 1
#define PASSIVE 0
#define CONTROL 0
#define WEIGHT_MESSAGE 1
#define MAXLINE 256

using namespace std;

std::mutex mtx;

class process{
public:
	int nodeid;
	int weight_num;
	int weight_den;
	int clocks;
	string ip;
	int status;
	int myParent;
	int isLeaf;
	int isInitiator;
	int noOfChildren;
	int noOfMessageSent;
	int maxSent;
	int noOfActiveProcesses;
	int delay;
	map<int,string> ip_info;
    vector<int> children;

    process(){
            ip = "";
            status = -1; // Set Invalid Status
            clocks = 0;
            weight_num = 0;
            weight_den = 0;
            noOfMessageSent = 0;
            maxSent = 0;
            myParent = -1; // Set Invalid Parent
            noOfChildren = -1; // Set Invalid Childs
            isInitiator = -1; //Set Invalid Value
            nodeid = -1; // Set Invalid ID
            isLeaf = 0; //Set Invalid Value
            noOfActiveProcesses = 0;
            delay = -1; //Set Invalid Value
        }
};

int gcd(int a, int b)
{
	a = a<0 ? -a : a;
	b = b<0 ? -b : b;
    if (a == 0)
        return b;
    return gcd(b%a, a);
}

void lowest(int &den3, int &num3)
{
    int common_factor = gcd(num3,den3);
    den3 = den3/common_factor;
    num3 = num3/common_factor;
}

void addFraction(int num1, int den1, int num2, int den2, int &num3, int &den3)
{
	  cout<< den1;
	  cout<<den2;
	  cout<<num1;
	  cout<<num2;
    den3 = gcd(den1,den2);

    den3 = (den1*den2) / den3;
    num3 = (num1)*(den3/den1) + (num2)*(den3/den2);
    lowest(den3,num3);
}


void testSend(process &Test, int processID, int msgType, int num, int den, int clocks){
      cout <<  "inside testSend, process to send msg is:" <<  processID << endl;
       int *buffer, elements;
    // Create msg
        if (msgType == WEIGHT_MESSAGE){
            //int buffer[4], elements = 4;
            buffer = new int[6], elements = 6;
            buffer[0] = WEIGHT_MESSAGE;
            buffer[1] = num;
            buffer[2] = den;
            buffer[3] = clocks;
            buffer[4] = Test.nodeid;
            buffer[5] = -1;
            mtx.lock();
            Test.noOfMessageSent = Test.noOfMessageSent + 1;
            mtx.unlock();
            cout << "Pid" << Test.nodeid << "sent app msg to :" << processID << endl;
        }
        else if(msgType == CONTROL){
            //int buffer[3], elements = 3;
            buffer = new int[6], elements = 6;
            buffer[0] = CONTROL;
            buffer[1] = num;
            buffer[2] = den;
            buffer[3] = clocks;
            buffer[4] = Test.nodeid;
            buffer[5] = -1;
            cout << "Pid" << Test.nodeid << "sent control msg to :" << processID << " with weight " << num << ":" << den << endl;
        }
          //cout <<  "inside mid of testSend"<< endl;

        int servPort = PORTNO;
        string serv_IP = Test.ip_info[processID];

        //cout <<  endl <<  "ip to send is :" <<  serv_IP;
        // std::string str;
        const char * servIP = serv_IP.c_str();

        // int senderID = topology[RandIndex];
        //
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
        unsigned int microseconds = rand()% 1000000 + 2000000;
        while(connect(sockfd, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0) {
            perror("connect() failed");
            usleep(microseconds);
        }
        //cout <<  "connect success";
        int length = sizeof(int) *(elements);
        int sentLen = send(sockfd, buffer, length, 0);
        if (sentLen < 0) {
            perror("send() failed");
            exit(-1);
        } else if (sentLen != length) {
            perror("send(): sent unexpected number of bytes");
            exit(-1);
        }
        close(sockfd);

}

void testSendEvents(process &Test, int delay){
  //cout <<  "inside beginning of testSendEvents"<< endl;
  if(Test.isLeaf!=1){
    //cout << " I am going to enter the loop" << endl;
    for (int i = Test.noOfMessageSent; i < Test.maxSent; ++i){
           //cout << "In loop" << endl;
            int choose = rand()%(Test.children.size());
            int chosen = Test.children.at(choose);
            int num_to_send, den_to_send;
            mtx.lock();
            num_to_send = -1;
            den_to_send = Test.weight_den*2;
            int num3, den3;
            addFraction(Test.weight_num,Test.weight_den, num_to_send, den_to_send, num3, den3);
            Test.weight_den = den3;
            Test.weight_num = num3;
            mtx.unlock();

            testSend(Test, chosen, WEIGHT_MESSAGE, -num_to_send, den_to_send, Test.clocks);
         }
     }
   else{
     for (int i = Test.noOfMessageSent; i < Test.maxSent; ++i){
        int num_to_send, den_to_send;
        mtx.lock();
        num_to_send = -1;
        den_to_send = Test.weight_den*2;
        int num3, den3;
        addFraction(Test.weight_num,Test.weight_den, num_to_send, den_to_send, num3, den3);
        Test.weight_den = den3;
        Test.weight_num = num3;
        mtx.unlock();
        testSend(Test, Test.myParent, WEIGHT_MESSAGE, -num_to_send, den_to_send, Test.clocks);
     }
   }
    //cout <<  "inside mid of testSendEvents"<< endl;
    mtx.lock();
    Test.status = PASSIVE;
    mtx.unlock();
    if(Test.isInitiator!= 1){
        testSend(Test, Test.myParent, CONTROL, Test.weight_num, Test.weight_den, 0);
        mtx.lock();
        Test.weight_num = 0;
        Test.weight_den = 1;
        mtx.unlock();
    }
      //cout <<  "inside end of testSendEvents"<< endl;
}

void recieveControlMessage(process &Test, int *buffer){
    int numerator = buffer[1];
    int denomniator = buffer[2];
    if(Test.isInitiator!=1){
    	testSend(Test, Test.myParent, CONTROL, numerator, denomniator, 0);
    }
    else{
    	  mtx.lock();
          int num3, den3;
          addFraction(Test.weight_num, Test.weight_den, numerator, denomniator, num3, den3);
          Test.weight_num = num3;
          Test.weight_den = den3;
          mtx.unlock();
          if ((Test.weight_num == Test.noOfActiveProcesses) && (Test.weight_den == 1) && Test.status == PASSIVE){
          	cout << "Final Weight :" << Test.weight_num << ":" << Test.weight_den;
            cout<< "Initiator detects Termination";
          	exit(1);
        }
    }
}

void recievedApplicationMessage(process &Test, int *buffer){
	int clocks = buffer[3];
    int num = buffer[1];
    int den = buffer[2];
	// mtx.lock();
 //    int num3, den3;
 //    addFraction(Test.weight_num, Test.weight_den, num, den, num3, den3);
 //    Test.weight_den = den3;
 //    Test.weight_num = num3;
 //    mtx.unlock();
	if(Test.isLeaf != 1 && Test.isInitiator != 1){
	  if(Test.status == PASSIVE){
		if(Test.noOfMessageSent < Test.maxSent){
			mtx.lock();
			Test.status = ACTIVE;
            int num3, den3;
            addFraction(Test.weight_num, Test.weight_den, num, den, num3, den3);
            Test.weight_den = den3;
            Test.weight_num = num3;
			Test.clocks = ( Test.clocks>clocks ? Test.clocks : clocks ) + 1;
			mtx.unlock();
			for (int i = Test.noOfMessageSent; i < Test.maxSent; ++i)
           {
            int choose = rand()%(Test.children.size());
            int chosen = Test.children.at(choose);
            int num_to_send, den_to_send;
            mtx.lock();
            num_to_send = -1;
            den_to_send = Test.weight_den*2;
            int num3, den3;
            addFraction(Test.weight_num,Test.weight_den, num_to_send, den_to_send, num3, den3);
            Test.weight_den = den3;
            Test.weight_num = num3;
            mtx.unlock();
            testSend(Test, chosen, WEIGHT_MESSAGE, -num_to_send, den_to_send, Test.clocks);
           }
           testSend(Test, Test.myParent,CONTROL,Test.weight_num,Test.weight_den, 0);
            mtx.lock();
            Test.weight_num = 0;
            Test.weight_den = 1;
            Test.status = PASSIVE;
            mtx.unlock();
		}
        else{
        	testSend(Test, Test.myParent, CONTROL, num, den, 0);
        }
     }
     else if(Test.status == ACTIVE){
			mtx.lock();
			Test.clocks = ( Test.clocks>clocks ? Test.clocks : clocks ) + 1;
            int num3, den3;
            addFraction(Test.weight_num, Test.weight_den, num, den, num3, den3);
            Test.weight_den = den3;
            Test.weight_num = num3;
            mtx.unlock();
	  }

 }
	  else if(Test.isLeaf==1){
	  if(Test.status == PASSIVE){
			if(Test.noOfMessageSent < Test.maxSent){
				mtx.lock();
				Test.status = ACTIVE;
                int num3, den3;
                addFraction(Test.weight_num, Test.weight_den, num, den, num3, den3);
                Test.weight_den = den3;
                Test.weight_num = num3;
				Test.clocks = ( Test.clocks>clocks ? Test.clocks : clocks ) + 1;
				mtx.unlock();
				for (int i = Test.noOfMessageSent; i < Test.maxSent; ++i)
	           {
	            int num_to_send, den_to_send;
	            mtx.lock();
	            num_to_send = -1;
	            den_to_send = Test.weight_den*2;
	            int num3, den3;
	            addFraction(Test.weight_num,Test.weight_den, num_to_send, den_to_send, num3, den3);
	            Test.weight_den = den3;
	            Test.weight_num = num3;
	            mtx.unlock();
	            testSend(Test, Test.myParent, WEIGHT_MESSAGE, -num_to_send, den_to_send, Test.clocks);
	           }
	           mtx.lock();
	           testSend(Test, Test.myParent,CONTROL,Test.weight_num,Test.weight_den, 0);
	           Test.weight_num = 0;
	           Test.weight_den = 1;
               Test.status = PASSIVE;
	           mtx.unlock();
			}
	        else{
	        	testSend(Test, Test.myParent, CONTROL, num, den, 0);
	        }
	     }
	     else if(Test.status ==ACTIVE){
	     	mtx.lock();
			Test.clocks = ( Test.clocks>clocks ? Test.clocks : clocks ) + 1;
            int num3, den3;
            addFraction(Test.weight_num,Test.weight_den, num, den, num3, den3);
            Test.weight_den = den3;
            Test.weight_num = num3;
	        mtx.unlock();
	     }
	  }
    else if (Test.isInitiator == 1){
         if(Test.noOfMessageSent < Test.maxSent){
         	mtx.lock();
			Test.clocks = ( Test.clocks>clocks ? Test.clocks : clocks ) + 1;
            int num3, den3;
            addFraction(Test.weight_num,Test.weight_den, num, den, num3, den3);
            Test.weight_den = den3;
            Test.weight_num = num3;
			mtx.unlock();
         }
         else{
            mtx.lock();
            int num3, den3;
            addFraction(Test.weight_num,Test.weight_den, num, den, num3, den3);
            Test.weight_den = den3;
            Test.weight_num = num3;
            mtx.unlock();
         }
         if ((Test.weight_num == Test.noOfActiveProcesses) && (Test.weight_den == 1) && Test.status == PASSIVE){
         	cout << "Final Weight :" << Test.weight_num << ":" << Test.weight_den;
            cout << "Initiator announces Termination";
         	exit(1);
         }
    }

}


void testRecvEvents(process &Test, int processID){

    // Run your server socket connection, for infinite amount of time to keep accepting the connections
    //cout <<  "inside: testRecvEvents : Server Code"<<  endl;
    //cout <<  "Pid to receive the msg is :" <<  processID <<  endl;

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
        if (buffer[0] == WEIGHT_MESSAGE){
             //cout << "Application msg is received, Start RecievedApplicationMessage thread" <<  endl;
             t3 = std::thread(recievedApplicationMessage, std::ref(Test), std::ref(buffer));
             t3.join();
        }
        if (buffer[0] == CONTROL){
            //cout <<  "Control msg is received, Start RecievedControlMessage thread" <<  endl;
            t4 = std::thread(recieveControlMessage, std::ref(Test), std::ref(buffer));
            t4.join();
        }

        close(clntSock);
               //cout <<  "inside end of testRecvEvents"<< endl;
    }
}



int main(int argc, char **argv){

    process ps;
    int myid, isinit, numberofnodes, minmsglimit ,maxmsglimit ,delay ,activeprocess;
    myid = atoi(argv[1]);
    isinit = atoi(argv[2]);

    ifstream infile("inp-params.txt");
    infile >> numberofnodes >> minmsglimit >> maxmsglimit >> delay >> activeprocess;
    infile.close();

    // Initializing some values of the class processobject
    ps.nodeid = myid;
    ps.isInitiator = isinit;
    ps.delay = delay;
    ps.noOfActiveProcesses = activeprocess;
    int range = maxmsglimit - minmsglimit + 1;
    srand(time(NULL));
    int maxSentsize = (rand() % range) + minmsglimit;
    ps.maxSent = maxSentsize;

   ifstream inFile ("topology.txt");
   char oneline[MAXLINE];
   const char s[3] = "-:";
   char *token;
   int n;
   inFile >> n;
   std::vector<std::string> temp; // temp vector
   //map<int,string> ip;
   // std::tempector<int> children;
   string sform;
   while (inFile)
   {
       inFile.getline(oneline, MAXLINE);
       token = strtok(oneline, s);
       while( token != NULL )
        {
          char *st;
          st = token;
          std::string sform(st);
          temp.push_back(sform);
          token = strtok(NULL, s);
        }
   }
   inFile.close();
   for (int i = 0; i < 3*n; i=i+3)
   {
      ps.ip_info.insert(pair<int, string>(stoi(temp.at(i)),temp.at(i+1)));
   }
   for (int i = 0; i <3*n; i=i+3){
    if(stoi(temp.at(i))==myid){
        ps.status = stoi(temp.at(i+2));
    }
   }

   map<int, string>::iterator it;
   for ( it = ps.ip_info.begin(); it != ps.ip_info.end(); it++ )
  {
    std::cout << it->first  // string (key)
              << ':'
              << it->second   // string's tempalue
              << std::endl ;
  }
  cout<<"-------------------"<<endl;


inFile.open("topology.txt");
for (int i =0; i <= n; i++){
   string s;
   getline(inFile, s);
}
int value;
char ch;
while(inFile){
   inFile.get(ch);
   value = ch -48;
   if (value == myid)
   {
      int x;
      while (inFile.peek() != '\n' && inFile >> x)
      {
         ps.children.push_back(x);
      }

   }
   string s;
   getline(inFile, s);
}
inFile.close();
for (int i = 0; i < ps.children.size(); ++i)
   {
      cout<<"at position ["<<i<<"]="<<ps.children.at(i)<<endl;
   }

// Get the Parent


inFile.open("topology.txt");
for (int i =0; i <= n; i++){
   string s;
   getline(inFile, s);

}

while(inFile){
   inFile.get(ch);
   value = ch -48;
   if (value != myid)
   {
      int x;
      while (inFile.peek() != '\n' && inFile >> x)
      {
         if(x == myid)
            ps.myParent = value;
         //cout << "My parent is :" << value << endl;
      }

   }
   string s;
   getline(inFile, s);
}
ps.noOfChildren = ps.children.size();
if (ps.noOfChildren == 0)
ps.isLeaf = 1;
//ps.status =
inFile.close();

      if(ps.status == ACTIVE){
   ps.weight_num =1;
   ps.weight_den = 1;}
 else{
 	ps.weight_den =1;
 	ps.weight_num = 0;
 }

        cout << "ps.pid" << " " << ps.nodeid << endl;
        cout << "ps.status" <<" " << ps.status << endl;
        //cout << "ps.color" << " " << ps.color << endl;
        cout << "ps.maxSent"<< " " << ps.maxSent << endl;
        cout << "ps.myParent"<< " " << ps.myParent << endl;
        cout << "ps.noFChilds" << " " << ps.noOfChildren << endl;
        cout << "ps.initiator" << " " << ps.isInitiator << endl;
        cout << "ps.nofmsgsent" << " " << ps.noOfMessageSent << endl;
        cout << "ps.noOfActiveProcesses" << " " << ps.noOfActiveProcesses << endl;
        // cout << "map " << ps.v[1] << endl;
        // cout << "map " << ps.v[2] << endl;
        // cout << "map " << ps.v[4] << endl;
        cout << "ps.isLeaf" << " " << ps.isLeaf << endl;



	//Create the Threads
	std::thread t1, t2;
	//cout <<  "inside main"<< endl;

	t1 = std::thread(testRecvEvents, std::ref(ps), myid);
	std::this_thread::sleep_for(std::chrono::seconds(5));
	if(ps.status == ACTIVE){
	        cout <<  "processis initially Active" <<  endl;
	        t2=std::thread(testSendEvents, std::ref(ps), delay);
	        t2.join();
	}
	t1.join();
	return 0;
}
