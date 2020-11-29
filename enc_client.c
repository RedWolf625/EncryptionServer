#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>  // ssize_t
#include <sys/socket.h> // send(),recv()
#include <netdb.h>      // gethostbyname()



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


/**
* Client code
* 1. Create a socket and connect to the server specified in the command arugments.
* 2. Prompt the user for input and send that input as a message to the server.
* 3. Print the message received from the server and exit the program.
*/

// Error function used for reporting issues
void error(const char* msg) {
    perror(msg);
    return;
}

// Set up the address struct
void setupAddressStruct(struct sockaddr_in* address,
    int portNumber,
    char* hostname) {

    // Clear out the address struct
    memset((char*)address, '\0', sizeof(*address));

    // The address should be network capable
    address->sin_family = AF_INET;

    // Store the port number
    address->sin_port = htons(portNumber);

    // Get the DNS entry for this host name
    struct hostent* hostInfo = gethostbyname(hostname);
    if (hostInfo == NULL) {
        fprintf(stderr, "CLIENT: ERROR, no such host\n");
        exit(2);
    }

    // Copy the first IP address from the DNS entry to sin_addr.s_addr
    memcpy((char*)&address->sin_addr.s_addr,
        hostInfo->h_addr_list[0],
        hostInfo->h_length);
}

int main(int argc, char* argv[]) {

    /*
        Ints to hold the socketFD, portNumber, the chars written to the server overall, 
        the specific amount of chars sent from the plaintext/key string and the verification string, 
        the amount of chars read from the server, the combined length of the plaintext/key string,
        and the file descriptor for our open plaintext and key files
    */
    int socketFD, portNumber, plainWritten, sentPbits, sentKbits, charsRead, comLen, validFile = 0;

    // Disallowed characters
    char bad[45] = { 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i',  'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '$', '*', '!', '(', '#', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-'};
    
    // Char array for our plainText
    char plainText[70000];
    
    // Char array for our key
    char keyText[70005];
    
    // Array to hold the combined plaintext and key data
    char finArr[140020];
    
    // Char to add the plainText file name to for opening
    char plainPath[15] = "./";
    
    // Char to add the key file name to for opening
    char keyPath[10] = "./";
    
    // Char to hold our verification info in
    char verifArr[4];

    struct sockaddr_in serverAddress;

    // Check usage & args
    if (argc < 4) {
        fprintf(stderr, "USAGE: %s hostname port\n", argv[0]);
        exit(2);
    }

    // Create a socket
    socketFD = socket(AF_INET, SOCK_STREAM, 0);
    if (socketFD < 0) {
        error("CLIENT: ERROR opening socket");
        exit(2);
    }

    // Set up the server address struct
    setupAddressStruct(&serverAddress, atoi(argv[3]), "localhost");



    // Get input plaintext filename
    strcat(plainPath, argv[1]);

    // Get input key filename
    strcat(keyPath, argv[2]);

    // Open the plaintext file in read only mode
    validFile = open(plainPath, O_RDONLY);

    if (validFile == -1) {
        printf("open() failed on \"%s\"\n", plainPath);
        perror("In main()");
        exit(1);
    }

    // Read the plaintext file into the plainText char array and close the file
    ssize_t plainLen = read(validFile, plainText, sizeof(plainText));
    close(validFile);

    // Do the same with the key as we did for plaintext
    validFile = open(keyPath, O_RDONLY);

    if (validFile == -1) {
        printf("open() failed on \"%s\"\n", plainPath);
        perror("In main()");
        exit(1);
    }
    ssize_t keyLen = read(validFile, keyText, sizeof(keyText));
    close(validFile);
    
    /* 
       Remove the trailing newline character from the plainText and keyText char arrays.  
       Decrement the ints holding their len and hold the combined length in the comLen int
     */
    plainText[strcspn(plainText, "\n")] = '\0';
    plainLen--;
    keyText[strcspn(keyText, "\n")] = '\0';
    keyLen--;
    comLen = plainLen + keyLen;

    // Iterate through the plainText and make sure no disallowed chars exist in plainText.
    for (int i = 0; i < strlen(plainText); i++) {
        for (int j = 0; j < strlen(bad); j++)
        if (plainText[i] == bad[j]) {
            error("ERROR: input contains bad characters\n");
            exit(1);
        }
    }

    // Check to see if the key is too short
    if (keyLen < plainLen) {
        error("ERROR: key is too short\n");
        exit(1);
    }

    // Set up the verification array to tell the server this client is the encryption client
    strcat(verifArr, "enc1");

    // place both the plaintext and the key in the final array with an @ in between them
    strcat(finArr, plainText);
    strcat(finArr, "@");
    strcat(finArr, keyText);


    // Connect to server
    if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        error("CLIENT: ERROR connecting");
        exit(2);
    }
    
    /* 
        First send the verification string if we are connecting for the first time followed 
        By the combined plaintext/key string.  Count how many chars we've sent and stop once we've hit
        the length of the combined string
    */
    while (plainWritten < comLen) {
        if (plainWritten == 0) {
            sentKbits = send(socketFD, verifArr, strlen(verifArr), 0);
        }
        sentPbits = send(socketFD, finArr, strlen(finArr), 0);
        plainWritten = plainWritten + sentPbits;
    }
    if (plainWritten < 0) {
        error("CLIENT: ERROR writing to socket");
        exit(2);
    }

    // Get return message from server
    // Clear out the buffer again for reuse
    memset(finArr, '\0', sizeof(finArr));

    // Read data from the socket, leaving \0 at end
    charsRead = recv(socketFD, finArr, sizeof(finArr) - 1, 0);
    if (charsRead < 0) {
        error("CLIENT: ERROR reading from socket");
        exit(2);
    }
    printf("%s\n", finArr);

    // Close the socket
    close(socketFD);

    return 0;
}