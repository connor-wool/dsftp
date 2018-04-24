#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>

//c++ includes to make my life easier
#include <vector>
#include <string>
#include <iostream>

#include "dataStructures.cpp"

using namespace std;

#define SERVER_PORT 5432
#define MAX_LINE 80

//file management
//FILE *fp;
int FileDescriptor;

//socket management
struct hostent *hp;
struct sockaddr_in sin;
char *host;  //server hostname as a cstring
char *fname; //input filename as a cstring
char buf[MAX_LINE];
int s;
int slen;

//used for connection management
#define MESSAGE_BUFFER_MAX_SIZE 256
#define STARTING_SEQ_VALUE 1

int currentSequenceValue = STARTING_SEQ_VALUE;
vector<struct MessageBlock> messageBuffer;
int FILE_FINISHED_FLAG = 0;
int ALL_MSGS_ACKED_FLAG = 0;

void createSocket()
{
    /* translate host name into peer’s IP address */
    hp = gethostbyname(host);
    if (!hp)
    {
        fprintf(stderr, "Unknown host: %s\n", host);
        exit(1);
    }

    /* build address data structure */
    bzero((char *)&sin, sizeof(sin));
    sin.sin_family = AF_INET;
    bcopy(hp->h_addr, (char *)&sin.sin_addr, hp->h_length);
    sin.sin_port = htons(SERVER_PORT);

    /* active open */
    if ((s = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("Socket");
        exit(1);
    }

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 1000;

    if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
    {
        perror("SetSockOpt Error");
    }
}

void openFile()
{
    FileDescriptor = open(fname, O_RDONLY);
    if(FileDescriptor <= 0){
        printf("Can't open file\n");
        exit(1);
    }
}

void fillMessageBuffer()
{
    cout << "Filling Message Buffer, current size: " << messageBuffer.size() << endl;
    char linebuf[1000];
    if (ALL_MSGS_ACKED_FLAG)
    {
        for (int i = 0; i < 10; i++)
        {
            struct MessageBlock m;
            memset(&m, 0, sizeof(struct MessageBlock));
            m.sequenceNumber = -1;
            m.flags = m.flags & MESSAGE_IS_TEARDOWN;
            messageBuffer.push_back(m);
        }
    }
    else
    {
        while (messageBuffer.size() < MESSAGE_BUFFER_MAX_SIZE)
        {
            int bytesRead = read(FileDescriptor, linebuf, 1000);
            if (bytesRead > 0)
            {
                struct MessageBlock m;
                memset(&m, 0, sizeof(struct MessageBlock));
                memcpy(m.message, linebuf, 1000);
                m.sequenceNumber = currentSequenceValue;
                m.messageSize = bytesRead;
                messageBuffer.push_back(m);
                currentSequenceValue++;
            }
            else
            {
                FILE_FINISHED_FLAG = 1;
                break;
            }
        }
    }
    cout << "Finished filling message buffer, new size: " << messageBuffer.size() << endl;
}

void sendMessages()
{
    cout << "sending messages" << endl;
    int sendStatus;
    socklen_t sock_len = sizeof sin;
    char sendBuffer[sizeof(struct MessageBlock)];
    for (int loop = 0; loop < 3; loop++)
    {
        cout << "Message pass" << endl;
        for (int i = 0; i < messageBuffer.size(); i++)
        {
            //get message from vector
            struct MessageBlock m = messageBuffer[i];
            memcpy(sendBuffer, &m, sizeof(struct MessageBlock));
            sendStatus = sendto(s, sendBuffer, sizeof(struct MessageBlock), 0, (struct sockaddr *)&sin, sock_len);
            perror("Sent message\n");
            if (i + 1 % 10 == 0)
            {
                cout << endl;
            }

            if (sendStatus == -1)
            {
                perror("SendTo Error\n");
                exit(1);
            }
        }
    }
    cout << "all messages sent" << endl;
}

void listenForAck()
{
    int recvStatus;
    char recvBuffer[sizeof(struct AckBlock)];
    recvStatus = recvfrom(s, recvBuffer, sizeof(recvBuffer), 0, 0, 0);
    cout << "entering wait loop for ack" << endl;
    while (recvStatus < 1)
    {
        recvStatus = recvfrom(s, recvBuffer, sizeof(recvBuffer), 0, 0, 0);
    }
    
    cout << "recieved ack, recvStatus = " << recvStatus << endl;
    cout << "sleeping to wait for full transmission" << endl;
    sleep(5);
    char deadBuffer[sizeof(struct AckBlock)];
    while(recvStatus > 0){
        recvStatus = recvfrom(s, deadBuffer, sizeof(recvBuffer), 0, 0, 0);
    }

    //need to clear out the recv socket first

    struct AckBlock a;
    memcpy(&a, recvBuffer, sizeof(struct AckBlock));
    for (int i = 0; i < sizeof(a.AckBuf); i++)
    {
        if (a.AckBuf[i] > 0)
        {
            int searchNum = a.AckBuf[i];
            for (int j = 0; j < messageBuffer.size(); j++)
            {
                if (messageBuffer[j].sequenceNumber == searchNum)
                {
                    cout << "removing message #" << j << endl;
                    messageBuffer.erase(messageBuffer.begin() + j);
                    break;
                }
            }
        }
    }
    if (FILE_FINISHED_FLAG && (messageBuffer.size() == 0))
    {
        ALL_MSGS_ACKED_FLAG = 1;
    }
}

int main(int argc, char *argv[])
{
    //usage: ./client serverPort fileToSend
    if (argc == 3)
    {
        host = argv[1];
        fname = argv[2];
    }
    else
    {
        fprintf(stderr, "Usage: ./client_udp host filename\n");
        exit(1);
    }

    createSocket();
    openFile();

    while (!ALL_MSGS_ACKED_FLAG)
    {
        fillMessageBuffer();
        sendMessages();
        listenForAck();
    }

    close(FileDescriptor);
}
