#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include <string>
#include <vector>
#include <iostream>
#include <fstream>

#include "dataStructures.cpp"

using namespace std;

#define SERVER_PORT 5432
#define MAX_LINE 256

//file control
char *fname;
FILE *fp;
char buf[MAX_LINE];

int FileDescriptor;

//socket control
struct sockaddr_in sin;
int len;
int s, i;
struct timeval tv;
char seq_num = 1;

//reliability control
vector<struct MessageBlock> messageBuffer;
vector<struct AckBlock> ackBuffer;
int currentBlockNumber = 1;
char recvdBlockTable[4096];

void init()
{
    memset(recvdBlockTable, 0, sizeof(recvdBlockTable));
}

void buildSocket()
{
    /* build address data structure */
    bzero((char *)&sin, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons(SERVER_PORT);

    /* setup passive open */
    if ((s = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("simplex-talk: socket");
        exit(1);
    }
    if ((bind(s, (struct sockaddr *)&sin, sizeof(sin))) < 0)
    {
        perror("simplex-talk: bind");
        exit(1);
    }

    socklen_t sock_len = sizeof sin;
}

void openFile()
{
    FileDescriptor = open(fname, O_WRONLY);
    if (FileDescriptor <= 0)
    {
        printf("Can't open file!\n");
        exit(1);
    }
}

//how do we know when the messages are done?
void listenForMessages()
{
    int messageLength;
    char recvBuffer[sizeof(struct MessageBlock)];
    socklen_t sock_len = sizeof sin;

    while (1)
    {
        messageLength = recvfrom(s, recvBuffer, sizeof(struct MessageBlock), 0, (struct sockaddr *)&sin, &sock_len);
        cout << "message length: " << messageLength << endl;
        if (messageLength > 0)
        {
            struct MessageBlock m;
            memcpy(&m, recvBuffer, sizeof(struct MessageBlock));
            messageBuffer.push_back(m);
            cout << "valid message!" << endl;
        }
    }
}

void buildAckMessages()
{
    cout << "building ack messages" << endl;
    cout << "ack message buffer size: " << ackBuffer.size() << endl;
    while (ackBuffer.size() < 10)
    {
        ackBuffer.push_back(ackBuffer.back());
    }
}

void writeMessageToFile()
{
    struct AckBlock a;
    memset(&a, 0, sizeof(struct AckBlock));
    a.numAcks = 0;
    char writeArray[1000];
    while (messageBuffer.size() > 0)
    {
        struct MessageBlock m = messageBuffer.back();
        int indexValue = m.sequenceNumber - 1;
        //kc wang's mailman algorithm?
        int previouslyWritten = (recvdBlockTable[indexValue / 8] >> (7 - (indexValue % 8))) & 0x01;
        if (!previouslyWritten)
        {
            memcpy(writeArray, m.message, m.messageSize);
            lseek(FileDescriptor, 0, (m.sequenceNumber - 1) * 1000);
            write(FileDescriptor, writeArray, m.messageSize);
            recvdBlockTable[indexValue / 8] = recvdBlockTable[indexValue / 8] | (0x01 << (7 - (indexValue % 8)));
            a.AckBuf[a.numAcks] = m.sequenceNumber;
            a.numAcks++;
        }
        messageBuffer.pop_back();
    }
    ackBuffer.push_back(a);
}

int main(int argc, char *argv[])
{
    if (argc == 2)
    {
        fname = argv[1];
    }
    else
    {
        fprintf(stderr, "usage: ./server_udp filename\n");
        exit(1);
    }

    init();
    buildSocket();
    openFile();
    while (1)
    {
        listenForMessages();
        writeMessageToFile();
        buildAckMessages();
    }
    close(FileDescriptor);
    close(s);
    /*
    //build address data structure
    bzero((char *)&sin, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons(SERVER_PORT);

    //setup passive open
    if ((s = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("simplex-talk: socket");
        exit(1);
    }
    if ((bind(s, (struct sockaddr *)&sin, sizeof(sin))) < 0)
    {
        perror("simplex-talk: bind");
        exit(1);
    }

    socklen_t sock_len = sizeof sin;

    fp = fopen(fname, "w");
    if (fp == NULL)
    {
        printf("Can't open file\n");
        exit(1);
    }

    int nextSeqNum = 1;
    string line;
    fstream outfile;
    outfile.open(fname);
    char replybuf[80];

    while (1)
    {
        len = recvfrom(s, buf, sizeof(buf), 0, (struct sockaddr *)&sin, &sock_len);

        if (buf[0] == -1)
        {
            printf("Transmission Complete\n");
            fclose(fp);
            close(s);
            exit(0);
        }

        line = buf;
        int currentSeqNum = (int)line.at(0);
        if (currentSeqNum == nextSeqNum)
        {
            nextSeqNum++;
            cout << "Sequence number matches expected!" << endl;
            memset(replybuf, 0, 80);
            replybuf[0] = (int)currentSeqNum;
            sendto(s, replybuf, 80, 0, (struct sockaddr *)&sin, sock_len);
            //good
        }
        else
        {
            cout << "!+! Sequence Error Mismatch: got " << currentSeqNum << " expected " << nextSeqNum << endl;
            memset(replybuf, 0, 80);
            replybuf[0] = (int)nextSeqNum;
            sendto(s, replybuf, 80, 0, (struct sockaddr *)&sin, sock_len);
            continue;
            //bad, dropped packet somewhere
            //handle out of order packet
        }

        line.erase(0, 1);

        if (len == -1)
        {
            perror("PError");
        }
        else if (len == 1)
        {
            if (buf[0] == 0xff && buf[1] == 0xff)
            {
                printf("Transmission Complete\n");
                break;
            }
            else
            {
                perror("Error: Short packet\n");
            }
        }
        else if (len > 1)
        {
            outfile << line;
            outfile.flush();
            //if (fputs((char *)line.c_str(), fp) < 1)
            //{
            //   printf("fputs() error\n");
            //}
        }
    }
    */
}
