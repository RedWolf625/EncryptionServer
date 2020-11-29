#include <stdlib.h>
#include <stdio.h>
#include <time.h>



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
  I used the following to generate a random number for this key generator: https://www.geeksforgeeks.org/generating-random-number-range-c/
*/

// Generate and return a random number in range l - r
int generate_random(int l, int r) {
    int rand_num = (rand() % (r - l + 1)) + l;
    return rand_num;
}

// Call our helper function to generate a random number from 0 - 26 and match that with the letter in the alphabet to build a random string
int main(int argc, char *argv[])
{
    // Our alphabet + space for key generation
    char alpha[27] = {'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z',' '};

    // The length of our key will be the second argument passed in from the console
    char *numb = argv[1];

    /* 
        Establish the upper and lower bounds for generate_random with an int for the desired len of 
        our key and an int for which index we are creating a char for
    */
    int lower = 0, upper = 26, count = atoi(numb), curr = 0;

    // Our array to hold our key +1 to hold the null terminator
    char strn[count+1];

    // Check to make sure we have enough args
    if (argc < 2) {
        fprintf(stderr, "Insufficient Argument\n", argv[0]);
        exit(1);
    }

    //current time as seed of random number generator
    srand(time(0)); 

    // generate count number of random chars and coppy them into the strn array
    for (int i = 0; i < count; i++) {
        curr = generate_random(lower, upper);
        strn[i] = alpha[curr];
    }

    // Set the last char to be a null terminator
    strn[count + 1] = '\0';

    // Print out the key with a newline char at the end
    for (int i = 0; i < count; i++) {
        printf("%c", strn[i]);
    }
    printf("\n");
    return 0;
}

