#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <semaphore.h>


/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
|Author: Nathan Shelby                                                                                                          |
|Date: 11/15/2020                                                                                                               |
|Course: CS344                                                                                                                  |
|                                        Homework 4: Multi-threaded Producer Consumer Pipeline                                  |
|                                                                                                                               |
|Write a program that creates 4 threads to process input from standard input as follows:                                        |
                                                                                                                                |
|Thread 1, called the Input Thread, reads in lines of characters from the standard input.                                       |
|Thread 2, called the Line Separator Thread, replaces every line separator in the input by a space.                             |
|Thread, 3 called the Plus Sign thread, replaces every pair of plus signs, i.e., "++", by a "^".                                |
|Thread 4, called the Output Thread, write this processed data to standard output as lines of exactly 80 characters.            |
|Furthermore, in your program these 4 threads must communicate with each other using the Producer-Consumer approach.            |                                                             |
|                                                                                                                               |
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

/*
  I used the final example from the explorations as the foundation for how I built out this program, modifying it to act as my input
  and output.  So any similarities you see between those two processes and the final example from Exploration: Condition Varialbes is
  for that reason.  I also utilized stack overflow for direction on how best to implement the character expansion.
*/



int connectionSocket, charsRead, sentBits, sendFbits;

struct sockaddr_in serverAddress, clientAddress;

socklen_t sizeOfClientInfo = sizeof(clientAddress);


// Number of resources in the pool
#define POOL_SIZE       5

// Declare a semaphone
sem_t counting_sem;

// Number of threads to spawn
int NUM_THREADS = 0;

// Error function used for reporting issues
void error(const char* msg) {
    perror(msg);
    exit(1);
}

void use_resource(int connectionSocket) {
    char buffer[140020];
    char test[11];
    char* key;
    char alpha[27] = { 'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z',' ' };
    char plainText[70000];
    char keyText[70005];

    // Get the message from the client and display it
    memset(test, '\0', 11);
    // Read the client's message from the socket
    charsRead = recv(connectionSocket, test, 11, 0);
    if (charsRead < 0) {
        error("ERROR reading from socket");
    }

    if (strstr(test, "dec1") == NULL) {
        error("ERROR you are not dec_client");
    }

    // Get the message from the client and display it
    memset(buffer, '\0', 140020);
    // Read the client's message from the socket
    charsRead = recv(connectionSocket, buffer, 140020, 0);
    if (charsRead < 0) {
        error("ERROR reading from socket");
    }

    key = strchr(buffer, '@');
    strcpy(keyText, key + 1);
    *key = 0;
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
        fInt = pInt - kInt;
        if (fInt < 0) {
            fInt = fInt + 27;
        }
        plainText[i] = alpha[fInt];
    }
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

void* dec_process(void* argument, int sock) {
    // The argument to the thread is a pointer to a character 

    sem_wait(&counting_sem);

    use_resource(sock);

    sem_post(&counting_sem);

    return NULL;
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



// Initiate our program, create and call all 4 threads and handle their return
int main(int argc, char* argv[]) {

    int pid;

    // Check usage & args
    if (argc < 2) {
        fprintf(stderr, "USAGE: %s port\n", argv[0]);
        exit(1);
    }

    // Initialize the semaphore to POOL_SIZE
    int res = sem_init(&counting_sem, 0, POOL_SIZE);

    pthread_t threads[NUM_THREADS];
    char thread_name[NUM_THREADS];

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

        pid = fork();

        if (pid < 0) {
            error("ERROR on fork");
        }

        if (pid == 0) {
            // Close the listening socket
            close(listenSocket);
            NUM_THREADS++;
            // Give threads a name starting with 'A' which is ASCII 65
            thread_name[NUM_THREADS] = NUM_THREADS + 65;
            dec_process((void*)&thread_name[NUM_THREADS], connectionSocket);
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
