/* test.cpp
A file used for testing various functionality of the program
*/
#include "dataStructures.h"
#include <iostream>
#include <string.h>

using namespace std;

int main(){
    struct MessageBlock m1, m2, m3;
    m1.sequenceNumber = 1;
    strcpy(m1.message, "Hello world, I'm m1!");

    m2.sequenceNumber = 2;
    strcpy(m2.message, "Hello world, I'm m2!");

    m1 = setBlockHash(m1);
    m2 = setBlockHash(m2);

    cout << "testing" << endl;
    cout << "m1 hash matches " << checkBlockHash(m1) << endl;
    cout << "m2 hash matches " << checkBlockHash(m2) << endl;

    cout << "Setting m1 as final message" << endl;
    setMessageFinal(&m1);
    cout << "Is m1 final message?: ";
    if(checkMessageFinal(&m1)){
        cout << "true" << endl;
    }
    else
    {
        cout << "false" << endl;
    }
}