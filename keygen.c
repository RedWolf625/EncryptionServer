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
  I used the final example from the explorations as the foundation for how I built out this program, modifying it to act as my input
  and output.  So any similarities you see between those two processes and the final example from Exploration: Condition Varialbes is
  for that reason.  I also utilized stack overflow for direction on how best to implement the character expansion.
*/

int generate_random(int l, int r) { //this will generate random number in range l and r
    int rand_num = (rand() % (r - l + 1)) + l;
    return rand_num;
}

// Initiate our program, create and call all 4 threads and handle their return
int main(int argc, char *argv[])
{
    char alpha[27] = {'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z',' '};
    char *numb = argv[1];
    int lower = 0, upper = 26, count = atoi(numb), curr = 0;
    char strn[count+1];

    // Check usage & args
    if (argc < 2) {
        fprintf(stderr, "USAGE: %s port\n", argv[0]);
        exit(1);
    }

    srand(time(0)); //current time as seed of random number generator
    for (int i = 0; i < count; i++) {
        curr = generate_random(lower, upper);
        strn[i] = alpha[curr];
    }
    strn[count + 1] = '\0';
    for (int i = 0; i < count; i++) {
        printf("%c", strn[i]);
    }
    printf("\n");
    return 0;
}

