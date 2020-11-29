#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <semaphore.h>


/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
|Author: Nathan Shelby                                                                                                             |
|Date: 11/28/2020                                                                                                                  |
|Course: CS344                                                                                                                     |
|                                        Homework 5: One-Time Pads                                                                 |
|                                                                                                                                  |
| Write programs that encrypt and decrypt plaintext files using a randomly generated key                                           |
| The programs are as follows:                                                                                                     |
| keygen will generate a random key of x length using the 26 capital letters and a space                                           |
| enc_client will send a plaintext file and a key using a network socket to enc_server. It will print the response from enc_server |
| enc_server will encrypt the plaintext using the key sent from enc_client and send back the encrypted string to enc_client        |
| dec_client will do the same with the encrypted string as enc+client did with the plaintext                                       |
| dec_server will decrypt the encrypted string using the key sent over by dec_client                                               |
|                                                                                                                                  |
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/


/*
  I used the following to help adjust the starter code to handle multiple connections: https://www.tutorialspoint.com/unix_sockets/socket_server_example.htm
  I also used the example of a counting semaphore from the exploration to help handle that.
*/


/* 
   Ints for the connection socket address of the current thread, the number of chars read from the client, the number of chars sent to the client
   And the number of chars sent in the current send request
*/
int connectionSocket, charsRead, sentBits, sendFbits;

// Defines the struct for ockaddr_in
struct sockaddr_in serverAddress, clientAddress;

// Defines the len of clientAddress
socklen_t sizeOfClientInfo = sizeof(clientAddress);

// Number of resources in the pool
#define POOL_SIZE       5

// Declare a semaphone
sem_t counting_sem;

// Keep track of how many threads we have currently
int NUM_THREADS = 0;


// Error function used for reporting issues
void error(const char* msg) {
    perror(msg);
    exit(1);
}


/*
   The meat of the encryption server.  Take the plaintext and key sent by the encryption client, 
   And encrypt the plaintext with the key
*/
void enc_process(int connectionSocket) {

    // Holds the plaintext and key sent over from the client 
    char buffer[140020];

    // Holds the verification data sent over from the client
    char test[4];

    // Used to point to the dividing char between the plaintext and key in buffer
    char* key;

    // Alphabet + space used for encryption
    char alpha[27] = { 'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z',' ' };

    // Holds the encrypted string
    char plainText[70000];

    // Holds the key sent over from the client
    char keyText[70005];

    // Get the verification message from the client and put it it the test buffer
    memset(test, '\0', 4);

    // Read the client's message from the socket
    charsRead = recv(connectionSocket, test, 4, 0);
    if (charsRead < 0) {
        error("ERROR reading from socket");
    }

    // If the verification data does not match, return an error
    if (strstr(test, "enc1") == NULL) {
        error("ERROR you are not enc_client");
    }

    // Get the message from the client and display it
    memset(buffer, '\0', 140020);

    // Read the client's message from the socket
    charsRead = recv(connectionSocket, buffer, 140020, 0);

    if (charsRead < 0) {
        error("ERROR reading from socket");
    }
    
    // Find and point to the @ symbol in the string that separates the plaintext from the key
    key = strchr(buffer, '@');

    // Take everything after the @ as the key
    strcpy(keyText, key + 1);

    // Erase the @ and everything after it to leave just the plaintext in the buffer
    *key = 0;

    // Iterate over the buffer and key char by char using the alphabet to assign numerical values to PInt and kInt for each character
    for (int i = 0; i < strlen(buffer); i++) {
        int pInt, kInt, fInt = 0;
        for (int j = 0; j < strlen(alpha); j++) {
            if (alpha[j] == buffer[i]) {
                pInt = j;
            }
            if (alpha[j] == keyText[i]) {
                kInt = j;
            }
        }

        // add pInt to kInt to get the int of the final encrypted char.  If it's beyond the alphabet range, subtract 27
        fInt = pInt + kInt;
        if (fInt > 26) {
            fInt = fInt - 27;
        }
        // Build the encrypted string char by char in plainText
        plainText[i] = alpha[fInt];
    }

    // Send the encrypted string back to the client, only stopping once you've sent a strlen(plainText) amount of chars
    charsRead = strlen(plainText);
    while (sentBits < charsRead) {

        // Send a Success message back to the client
        sendFbits = send(connectionSocket, plainText, strlen(plainText), 0);
        sentBits = sentBits + sendFbits;
    }

    if (charsRead < 0) {
        error("ERROR writing to socket");
    }

    NUM_THREADS--;
    return;
}


// Set up the address struct for the server socket
void setupAddressStruct(struct sockaddr_in* address,
    int portNumber) {

    // Clear out the address struct
    memset((char*)address, '\0', sizeof(*address));

    // The address should be network capable
    address->sin_family = AF_INET;
    // Store the port number
    address->sin_port = htons(portNumber);
    // Allow a client at any address to connect to this server
    address->sin_addr.s_addr = INADDR_ANY;
}


// Initiate our program, create a socket, listen for connections, accept connections, and run the encryption method
int main(int argc, char* argv[]) {

    // Holds the pid for spun off child processes
    int pid;

    // Check usage & args
    if (argc < 2) {
        fprintf(stderr, "USAGE: %s port\n", argv[0]);
        exit(1);
    }

    // Initialize the semaphore to POOL_SIZE
    int res = sem_init(&counting_sem, 0, POOL_SIZE);


    // Create the socket that will listen for connections
    int listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    
    if (listenSocket < 0) {
        error("ERROR opening socket");
    }

    // Set up the address struct for the server socket
    setupAddressStruct(&serverAddress, atoi(argv[1]));

    // Associate the socket to the port
    if (bind(listenSocket,
        (struct sockaddr*)&serverAddress,
        sizeof(serverAddress)) < 0) {
        error("ERROR on binding");
    }

    // Start listening for connetions. Allow up to 5 connections to queue up
    listen(listenSocket, 5);

    // Accept a connection, blocking if one is not available until one connects
    while (1) {
        // Accept the connection request which creates a connection socket
        connectionSocket = accept(listenSocket,
            (struct sockaddr*)&clientAddress,
            &sizeOfClientInfo);

        if (connectionSocket < 0) {
            error("ERROR on accept");
        }

        // Fork off a child process
        pid = fork();

        if (pid < 0) {
            error("ERROR on fork");
        }

        // If successful, begin the encryption process
        if (pid == 0) {

            // Close the listening socket
            close(listenSocket);

            // Increase our thread tracking int by 1
            NUM_THREADS++;

            // Try to decrement the semaphore and run the use_resource function.  Blocks if it cannot decrement    
            sem_wait(&counting_sem);

            enc_process(connectionSocket);

            // Increment the semaphore
            sem_post(&counting_sem);

            exit(0);
        }
        else {
            close(connectionSocket);
        }

    }

    // Destroy the semphore
    sem_destroy(&counting_sem);
    return 0;
}
