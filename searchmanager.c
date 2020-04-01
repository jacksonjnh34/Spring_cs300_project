#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include<stdio.h> 
#include<signal.h> 
#include <errno.h>
#include <pthread.h> 
//#include <iostream>
#include "longest_word_search.h"
#include "queue_ids.h"

char **statuses;
char **globalPrefixes;
int numPrefixes = 0;

pthread_mutex_t lock;

size_t                  /* O - Length of string */
strlcpy(char       *dst,        /* O - Destination string */
        const char *src,      /* I - Source string */
        size_t      size)     /* I - Size of destination string buffer */
{
    size_t    srclen;         /* Length of source string */


    /*
     * Figure out how much room is needed...
     */

    size --;

    srclen = strlen(src);

    /*
     * Copy the appropriate amount...
     */

    if (srclen > size)
        srclen = size;

    memcpy(dst, src, srclen);
    dst[srclen] = '\0';

    return (srclen);
}

/*
//  A method to produce a substring of a given input
//  CString, with variables p, and l as the starting
//  position of the string and the length of the
//  substing respectively
*/
void substring(char s[], char sub[], int p, int l) 
{
   int c = 0;
   
   while (c < l) 
   {
      sub[c] = s[p+c-1];
      c++;
   }
   sub[c] = '\0';
}

/*
//  I only utilized one sigint handler withb global
//  variable updating, I found this to be the easiest
//  to comprehend
*/
void handle_SIGINT(int dummy)
{
    //Seperate the ^C characters from statuses
    printf(" ");
    for(int i = 0; i < numPrefixes; i++)
    {
        //Print from global variables
        printf("%s - %s\n", globalPrefixes[i], statuses[i]);
    }
    signal(SIGINT, handle_SIGINT);
}

int main(int argc, char**argv)
{    
    //Make sure that a prefix is given
    if(argc < 3)
    {
        fprintf(stderr, "Must provide a wait time and at least one prefix\n", errno);
        exit(-1);
    }

    //Initialize a mutex to protect global variable writes
    if(pthread_mutex_init(&lock, NULL) != 0)
    {
        fprintf(stderr, "Mutex lock failed\n", errno);
        exit(-1);
    }
    
    //Allocate global memory for population
    statuses = (char**) malloc(sizeof(char**) * argc - 2);
    globalPrefixes = (char**) malloc(sizeof(char**) * argc - 2);
    numPrefixes = argc - 2;

    //General setup for msg passing
    key_t key;
    char prefixes[argc-2][WORD_LENGTH];
    int msqid;
    int msgflg = IPC_CREAT | 0666;

    //Declare signal handler
    signal(SIGINT, handle_SIGINT);

    //Populate various arrays with prefixes and statuses
    for(int i = 0; i < argc-2; i++)
    {
        //Allocate memory for each individual Cstring
        statuses[i] = (char*) malloc(sizeof(char) * WORD_LENGTH);
        globalPrefixes[i] = (char*) malloc(sizeof(char) * WORD_LENGTH);

        //Make sure the prefixes are within the given bounds
        if(strlen(argv[i+2]) > 20 || strlen(argv[i+2]) < 3)
        {
            fprintf(stderr, "Prefixes must be less than 20 characters, and more than 3 characters\n", errno);
            exit(-1);
        }

        //Copy prefixes and statuses onto arrays
        pthread_mutex_lock(&lock);
        strlcpy(statuses[i], "pending", WORD_LENGTH);
        strlcpy(prefixes[i], argv[i+2], WORD_LENGTH);
        strlcpy(globalPrefixes[i], argv[i+2], WORD_LENGTH);
        pthread_mutex_unlock(&lock);
    }

    
    //General message passing setup for unique SystemV queue
    key = ftok(CRIMSON_ID,QUEUE_NUMBER);
    if ((msqid = msgget(key, msgflg)) < 0) 
    {
        int errnum = errno;
        fprintf(stderr, "Value of errno: %d\n", errno);
        perror("(msgget)");
        fprintf(stderr, "Error msgget: %s\n", strerror( errnum ));
    }

    //Main execution loop to perform message passing/recieveing and printing reports for each submitted prefix
    for(int j = 0; j < argc-2; j++)
    {
        //Setup initial variables for current loop iteration
        prefix_buf sbuf;
        sbuf.id = j+1;
        strcpy(sbuf.prefix, prefixes[j]);
        sbuf.mtype = 1;

        //Length of message to be sent
        size_t buffer_len = strlen(sbuf.prefix) + sizeof(int) + 1;

        //Send the actual message
        if((msgsnd(msqid, &sbuf, buffer_len, IPC_NOWAIT)) < 0) 
        {
            int errnum = errno;
            fprintf(stderr,"%d, %ld, %s, %d\n", msqid, sbuf.mtype, sbuf.prefix, (int)buffer_len);
            perror("(msgsnd)");
            fprintf(stderr, "Error sending msg: %s\n", strerror( errnum ));
            exit(1);
        }
        else
        {
            //Status of message sending when success
            printf("Message(%d): \"%s\" Sent (%d bytes)\n\n", sbuf.id, sbuf.prefix,(int)buffer_len);
        }

        //Return value setup variables
        int ret;
        response_buf rbuf;
        int responsesRecieved = 0;
        int totalPassages = 1;
        response_buf *responses;
        size_t *responseSizes;

        //Use a do while here so that the first loop will always populate the values necessary to continue to recieve all the responses for the prefix
        do 
        {
            //General message reception
            ret = msgrcv(msqid, &rbuf, sizeof(response_buf), 2, 0);//receive type 2 message
            int errnum = errno;
            if (ret < 0 && errno !=EINTR)
            {
                fprintf(stderr, "Value of errno: %d\n", errno);
                perror("Error printed by perror");
                fprintf(stderr, "Error receiving msg: %s\n", strerror( errnum ));
            }
            else
            {
                //MESSAGE RECIEVED
                //Increment the number of responses recieved, and using the first message, set the number of total passages to the value given by the PassageProcessor
                responsesRecieved++;
                totalPassages = rbuf.count;

                //On first message response allocate the space in the responses array for later report, (as well as cooresponding sizes array)
                if(responsesRecieved == 1)
                {
                    responses = (response_buf*)malloc(sizeof(response_buf) * totalPassages);
                    responseSizes = (size_t*)malloc(sizeof(size_t) * totalPassages);
                }

                //Populate report arrays with returned values
                responses[rbuf.index] = rbuf;
                responseSizes[rbuf.index] = ret;

                pthread_mutex_lock(&lock);
                //Set the status for interrupt handler
                char status[6];
                char totalPass[1];
                snprintf(status, 6, "%d of ", responsesRecieved);
                snprintf(totalPass, 1, "%d", totalPassages);
                strcat(status, totalPass);
                pthread_mutex_unlock(&lock);

                strlcpy(statuses[j], status, WORD_LENGTH);               
            }
        } while (((ret < 0 ) && (errno == 4)) || responsesRecieved < totalPassages);
        //This while loop implements the logic for the general message reception handler, as well as introducing logic to make sure ALL responses are recieved

        //Print out the report for the finished prefix
        printf("Report \"%s\"\n", sbuf.prefix);
        for(int k = 0; k < totalPassages; k++)
        {
            if(strlen(responses[k].location_description) > 30)
            {
                responses[k].location_description[30] = '\0';
            }
            if (responses[k].present == 1)
            {
                printf("Passage %d - %s - %s\n", responses[k].index, responses[k].location_description, responses[k].longest_word);
            }
            else
            {
                printf("Passage %d - %s - no word found\n", responses[k].index, responses[k].location_description);
            }
                
        }

        printf("\n");

        //Update the status for the prefix as 'done'
        pthread_mutex_lock(&lock);
        strlcpy(statuses[j], "done", WORD_LENGTH);
        pthread_mutex_unlock(&lock);

        //Sleep for given input time
        sleep(atoi(argv[1]));
    

    }

    //Send a basic message with prefix "   " and id=0 to tell the PassageProcessor to terminate
    prefix_buf finalSBuf;
    finalSBuf.id = 0;
    strlcpy(finalSBuf.prefix, "   ", WORD_LENGTH);
    finalSBuf.mtype = 1;
    size_t buffer_len = strlen(finalSBuf.prefix) + sizeof(int) + 1;
    if((msgsnd(msqid, &finalSBuf, buffer_len, IPC_NOWAIT)) < 0) 
    {
        int errnum = errno;
        fprintf(stderr,"%d, %ld, %s, %d\n", msqid, finalSBuf.mtype, finalSBuf.prefix, (int)buffer_len);
        perror("(msgsnd)");
        fprintf(stderr, "Error sending msg: %s\n", strerror( errnum ));
        exit(1);
    }
    else
    {
        printf("Message(%d): \"%s\" Sent (%d bytes)\n", finalSBuf.id, finalSBuf.prefix,(int)buffer_len);
    }

    //Exit program!
    printf("Exiting ...\n ");

    pthread_mutex_destroy(&lock);

}