#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>



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




int strr = 0;       // Int that acts as a flag for the input producer to wake up the line_separator consumer

int strstrp = 0;    // Int that acts as a flag for the line_separator producer to wake up the plus_sign consumer

int strin = 0;      // Int that acts as a flag for the plus_sign producer to wake up the output consumer

int count = 0;      // Int that indicates a string is currently being processed

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;      // Initialize the mutex

pthread_cond_t empty = PTHREAD_COND_INITIALIZER;        // Initialize the condition variable for input

pthread_cond_t string_in = PTHREAD_COND_INITIALIZER;    // Initialize the condition variable for line_separator

pthread_cond_t stripped = PTHREAD_COND_INITIALIZER;     // Initialize the condition variable for plus_sign

pthread_cond_t full = PTHREAD_COND_INITIALIZER;         // Initialize the condition variable for output

pthread_cond_t enc5 = PTHREAD_COND_INITIALIZER;   

int connectionSocket, charsRead, sentBits, sendFbits;

char buffer[140020];

char test[11];

struct sockaddr_in serverAddress, clientAddress;

socklen_t sizeOfClientInfo = sizeof(clientAddress);

int pCount = 0;

pid_t spawnpid;

// Error function used for reporting issues
void error(const char* msg) {
    perror(msg);
    exit(1);
}

void enc_process() {
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
        fInt = pInt + kInt;
        if (fInt > 26) {
            fInt = fInt - 27;
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
    // Close the connection socket for this client
    close(connectionSocket);
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

/*
 Function that the takes the input from stdin, calls the helper function to put it into a buffer,
 flags the count var, and notifies the line_separator function to proceed
*/
void* encp_thread1(void* args)
{

        // Lock the mutex before putting item in the buffer
        pthread_mutex_lock(&mutex);

        // If we already are processing a string, don't take any more
        if (count > 0) {
            pthread_cond_wait(&empty, &mutex);
        }

        // Run the helper function to put the input into the temp buffer
        enc_process();

        // Unlock the mutex
        pthread_mutex_unlock(&mutex);

        // Signal to line_separater to begin and raise the flag that a string is being processed
        pthread_cond_signal(&string_in);
        strr++;
        count++;

    
    return NULL;

}


/*
 Function that the takes the buffer from input, calls the helper function to replace the newline
 char, and notifies the plus_sign function to proceed
*/
void* encp_thread2(void* args)
{

        // Lock the mutex before checking if the buffer has data      
        pthread_mutex_lock(&mutex);
        while (strr == 0) {
            // We aren't ready to strip any '\n' yet
            pthread_cond_wait(&string_in, &mutex);
        }

        // Run the helper function to replace all '\n' with ' '
        enc_process();

        // Clear the flag for line_separator and signal the condition variable for plus_sign
        strr--;
        pthread_cond_signal(&stripped);
        strstrp++;

        // Unlock the mutex
        pthread_mutex_unlock(&mutex);
    
    return NULL;
}


/*
 Function that the takes the buffer from line_separator, calls the helper function to replace ++,
 counts the string to see if we've hit 8 chars, and notifies output function to proceed
*/
void* encp_thread3(void* args)
{

        // Lock the mutex before checking if we are ready to proceed     
        pthread_mutex_lock(&mutex);
        while (strstrp == 0) {
            // We aren't ready to strip any ++ yet
            pthread_cond_wait(&stripped, &mutex);
        }

        enc_process();
        strstrp--;
        pthread_cond_signal(&full);

        

        // Unlock the mutex
        pthread_mutex_unlock(&mutex);
    
    return NULL;
}


/*
 Function that takes the buffer from plus_sign and prints it to the console
*/
void* encp_thread4(void* args)
{

        // Lock the mutex before checking if the buffer has data      
        pthread_mutex_lock(&mutex);
        while (strin == 0)
            // Buffer is empty. Wait for the producer to signal that the buffer has data
            pthread_cond_wait(&full, &mutex);
        
        enc_process();

        // Signal input that we are ready for the next line
        pthread_cond_signal(&enc5);

        // Unlock the mutex
        pthread_mutex_unlock(&mutex);
    return NULL;
}

void* encp_thread5(void* args) {
    pthread_mutex_lock(&mutex);
    if (pCount) {
        pthread_cond_wait(&enc5, &mutex);
    }
    enc_process();
    pthread_cond_signal(&empty);
    count--;
    pthread_mutex_unlock(&mutex);
    return NULL;
}


// Initiate our program, create and call all 4 threads and handle their return
int main(int argc, char* argv[]) {


    // Check usage & args
    if (argc < 2) {
        fprintf(stderr, "USAGE: %s port\n", argv[0]);
        exit(1);
    }

    pthread_t enc_thread1, enc_thread2, enc_thread3, enc_thread4, enc_thread5;

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
        if (pCount == 0) {
            // If fork is successful, the value of spawnpid will be 0 in the child, the child's pid in the parent
            pCount++;
            printf("\nONE\n");
            // Create the input thread
            pthread_create(&enc_thread1, NULL, encp_thread1, NULL);
            pthread_join(enc_thread1, NULL);
            pCount--;
        }
        if (pCount == 1) {
            pCount++;
            // Wait for the threads to resolve
            printf("\nTWO\n");
            // Now create the line separator thread
            pthread_create(&enc_thread2, NULL, encp_thread2, NULL);
            pthread_join(enc_thread2, NULL);
            pCount--;
        }
        if (pCount == 2) {
            pCount++;
            printf("\nTHREE\n");
            // Now create the plus sign thread
            pthread_create(&enc_thread3, NULL, encp_thread3, NULL);
            pthread_join(enc_thread3, NULL);
            pCount--;
        }
        if (pCount == 3) {
            pCount++;
            printf("\nFOUR\n");
            // Now create the output thread
            pthread_create(&enc_thread4, NULL, encp_thread4, NULL);
            pthread_join(enc_thread4, NULL);
            pCount--;
        }
        if (pCount == 4) {
            pCount++;
            printf("\nFIVE\n");
            // Now create the output thread
            pthread_create(&enc_thread5, NULL, encp_thread5, NULL);
            pthread_join(enc_thread5, NULL);
            pCount--;
        }
        else {}

    }

    // Close the listening socket
    close(listenSocket);
    return 0;
}
