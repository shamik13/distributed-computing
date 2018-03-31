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
#define MAXLINE 256

std::mutex mtx;

// Msg Types
#define APPL 0
#define CONT 1

// Process State
#define PASSIVE 0
#define ACTIVE 1

// Token Colors and Signal
#define WHITE 0
#define BLACK 1
#define REPEAT 2
#define INVALID -1


using namespace std;

class Node{

    public:
        string ip;
        int clock;
        int myPid;
        int status; 
        int color; 
        int maxSent; 
        int noFSent; 
        int myParent; 
        int noFChilds; 
        int isInitiator;
        int noFTokensRecv;
        int isLeaf; 
        int delay;
        int FLAG; 
    
        map<int,string> ip_info;
        vector<int> children;
        Node(){
            ip = "";
            status = -1; 
            clock = 0;
            color = -1;
            maxSent = 0;
            noFSent = 0;
            myParent = -1;
            noFChilds = -1; 
            isInitiator = -1; 
            myPid = -1; 
            noFTokensRecv = 0;
            FLAG = 0;
            isLeaf = 0;
            delay = -1; 
        }

};



void testSend(Node &Test, int processID, int msgType, int subType);
int max(int a, int b){
    if (a > b)
        return a;
    else
        return b;
}

void testSendEvents(Node &Test, int delay){
  cout << "inside beginning of testSendEvents"<<endl;
  if(!Test.isLeaf){
    for (int i = Test.noFSent; i < Test.maxSent; ++i){

            int choose = rand()%(Test.children.size());
            int chosen = Test.children.at(choose);
            cout << "Chosen is: " << chosen << endl;
            testSend(Test, chosen, APPL, 0);
         }}
   else{
     for (int i = Test.noFSent; i < Test.maxSent; ++i){
        testSend(Test,Test.myParent,APPL,0);
     }
   }
      cout << "inside mid of testSendEvents"<<endl;

    if(Test.isLeaf == 1){
        mtx.lock();
        Test.status = PASSIVE;
        Test.color = BLACK;
        mtx.unlock();
        testSend(Test, Test.myParent, CONT, BLACK);
    }
    else if(Test.isInitiator == 1){
        Test.status = PASSIVE;
    }
    else{
       
       mtx.lock();
       Test.status = PASSIVE;
       Test.color = BLACK;
       mtx.unlock();
    }
      cout << "inside end of testSendEvents"<<endl;
}

// void testSendEvents(Node &Test, int processID, int msgType, int subType){
void testSend(Node &Test, int processID, int msgType, int subType){
      cout << "inside testSend, process to send msg is :" << processID<<" "<<subType<<endl;


        int *buffer, elements;
    // Create msg
        if (msgType == APPL){
            //int buffer[4], elements = 4;
            buffer = new int[4], elements = 4;
            buffer[0] = APPL;
            buffer[1] = Test.myPid;
            buffer[2] = Test.clock;
            buffer[3] = -1;
            mtx.lock();
            Test.noFSent = Test.noFSent + 1;
            Test.color = BLACK;
            mtx.unlock();
            //sendMsg();
        }
        else if(msgType == CONT){
            //int buffer[3], elements = 3;
            buffer = new int[3], elements = 3;
            buffer[0] = CONT;
            buffer[1] = subType;
            buffer[2] = -1;
        }
          cout << "inside mid of testSend"<<endl;

        int servPort = PORTNO;
        string serv_IP = Test.ip_info[processID];

        cout << endl << "ip to send is :" << serv_IP;
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
        cout << "connect success";
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

void RecievedControlMessage(Node &Test,int *buffer){
 //  cout << "inside  of RecievedControlMessage"<<endl;

 if(buffer[1]==REPEAT){
  cout<<"message recieved type is = "<<buffer[1]<<endl;
  if(Test.isLeaf){
    testSend(Test, Test.myParent, CONT, Test.color);
    }
  else if(!Test.isLeaf && !Test.isInitiator){
     for (int i = 0; i < Test.noFChilds; ++i)
     {

        testSend(Test,Test.children[i], CONT, REPEAT);
     }
  }
 }
 else if(buffer[1] == BLACK || buffer[1] == WHITE){
    cout<<"message recieved type is = "<<buffer[1]<<endl;

      if(!Test.isInitiator && !Test.isLeaf){
        if (buffer[1] == BLACK){
          mtx.lock();
          Test.FLAG = 1;
          mtx.unlock();
        }
       if (++Test.noFTokensRecv==Test.noFChilds){
        mtx.lock();
        Test.noFTokensRecv = 0;

        mtx.unlock();
        if(Test.status == PASSIVE){
        if (Test.FLAG == 1){
         testSend(Test,Test.myParent,CONT, BLACK);
         mtx.lock();
         Test.color = WHITE;
         Test.FLAG = 0;
         mtx.unlock();
        }
        else
         testSend(Test,Test.myParent,CONT, WHITE);
         }
       }
     }
   //   cout << "inside mid of RecievedControlMessage"<<endl;
     if (Test.isInitiator){
      if (buffer[1] == BLACK){
          mtx.lock();
          Test.FLAG = 1;
          mtx.unlock();
        }
       if(++Test.noFTokensRecv==Test.noFChilds){
        mtx.lock();
        Test.noFTokensRecv = 0;
        mtx.unlock();
        cout<<"initiator recieved all tokens and state of tokens is"<< Test.FLAG<<endl;
       if (Test.FLAG == 1){
         for (int i = 0; i < Test.noFChilds; ++i)
         {
           testSend(Test,Test.children[i],CONT, REPEAT);
         }
         mtx.lock();
         Test.FLAG = 0;
         mtx.unlock();
       }
       else{
          //if(Test.status == PASSIVE)
           while(!(Test.status == PASSIVE)){}
           cout<< "Initiator announced termination detection";
           exit(1);
        }
       }}
      }
 //      cout << "inside ending of RecievedControlMessage"<<endl;
 }
//}

void RecievedApplicationMessage(Node &Test, int *buffer){
     cout << "inside: RecievedApplicationMessage"<<endl;
    if (Test.isLeaf){
        cout << "I am leaf" << endl;
        if(Test.noFSent < Test.maxSent){
            if(Test.status == PASSIVE){
                mtx.lock();
                Test.status = ACTIVE;
                cout<< "NODE " << Test.myPid <<" BECOMES ACTIVE" << "by Node" << buffer[1] << endl;
                mtx.unlock();
             }
             Test.clock = max(Test.clock, buffer[2])+1;
             for (int i = Test.noFSent; i < Test.maxSent; ++i){
                testSend(Test, Test.myParent, APPL, 0);
             }
             mtx.lock();
            Test.status = PASSIVE;
            Test.color = BLACK;
            mtx.unlock();
             testSend(Test, Test.myParent, CONT, BLACK);
             cout << "I completed execution, send black token to my parent"<<endl;
             mtx.lock();
        Test.color = WHITE;
        mtx.unlock();
        cout << "I am white again"<< endl;
            }

        //testSend(Test, Test.myParent, CONT, BLACK);

    }
     else if(!Test.isLeaf){
       if(Test.noFSent < Test.maxSent){
         if (Test.status == PASSIVE){
            mtx.lock();
            Test.status = ACTIVE;
            cout<< "NODE " << Test.myPid <<" BECOMES ACTIVE";
            mtx.unlock();
         }
         for (int i = Test.noFSent; i < Test.maxSent; ++i)
         {

            int choose = rand()%(Test.children.size());
            int chosen = Test.children.at(choose);
            //int nodeID = random();
            testSend(Test, chosen, APPL, 0);
         }
       }
       mtx.lock();
       Test.status = PASSIVE;
       Test.color = BLACK;
       mtx.unlock();
     }
          cout << "inside end of RecievedApplicationMessage"<<endl;
}

void testRecvEvents(Node &Test, int processID){

   
   // cout << "inside: testRecvEvents : Server Code"<< endl;
    cout << "Pid to receive the msg is :" << processID << endl;

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
        if (buffer[0] == APPL){
            // cout << "Application msg is received, Start RecievedApplicationMessage thread" << endl;
             t3 = std::thread(RecievedApplicationMessage, std::ref(Test), std::ref(buffer));
             t3.join();
        }
        if (buffer[0] == CONT){
           // cout << "Control msg is received, Start RecievedControlMessage thread" << endl;
            t4 = std::thread(RecievedControlMessage, std::ref(Test), std::ref(buffer));
            t4.join();
        }

        close(clntSock);
               cout << "inside end of testRecvEvents"<<endl;


    }
}

int main(int argc, char **argv){
     Node ps;
    int myid, isinit, numberofnodes, minmsglimit ,maxmsglimit ,delay;    
    myid = atoi(argv[1]);
    isinit = atoi(argv[2]);

    ifstream infile("inp-params.txt");
    infile >> numberofnodes >> minmsglimit >> maxmsglimit >> delay;
    infile.close();

    // Initializing some values of the class processobject    
    ps.myPid = myid;   
    ps.isInitiator = isinit;
    ps.delay = delay;
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
ps.noFChilds = ps.children.size();
if (ps.noFChilds == 0){
ps.isLeaf = 1;
 ps.color = WHITE;
}
//ps.status = 
inFile.close();


        cout << "node.pid" << " " << ps.myPid << endl;
        cout << "node.status" <<" " << ps.status << endl;
        cout << "node.color" << " " << ps.color << endl;
        cout << "node.maxSent"<< " " << ps.maxSent << endl;
        cout << "node.myParent"<< " " << ps.myParent << endl;
        cout << "node.noFChilds" << " " << ps.noFChilds << endl;
        cout << "node.initiator" << " " << ps.isInitiator << endl;
        // cout << "map " << node.v[1] << endl;
        // cout << "map " << node.v[2] << endl;
        // cout << "map " << node.v[4] << endl;

    // Close the file
  

    // Create the Threads
    std::thread t1, t2;

    t1 = std::thread(testRecvEvents, std::ref(ps), myid);
    if(ps.status == ACTIVE){
    t2=std::thread(testSendEvents, std::ref(ps), delay);
   //t2.join();
    }
    t1.join();
    t2.join();

    return 0;
}
