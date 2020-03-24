#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
//#include <iostream>
#include "longest_word_search.h"
#include "queue_ids.h"


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

int main(int argc, char**argv)
{
    /*
    if (argc <= 2 || strlen(argv[2]) <2) 
    {
        printf("Error: please provide prefix of at least two characters for search\n");
        printf("Usage: %s <prefix>\n",argv[0]);
        exit(-1);
    }
    */
    printf("ARGC SIZE: %d\n", argc);

    key_t key;
    char prefixes[argc-2][WORD_LENGTH];
    int msqid;
    int msgflg = IPC_CREAT | 0666;

    for(int i = 0; i < argc-2; i++)
    {
        printf("Adding prefix: %s\n", argv[i+2]);
        strlcpy(prefixes[i], argv[i+2], WORD_LENGTH);
    }

    key = ftok(CRIMSON_ID,QUEUE_NUMBER);
    if ((msqid = msgget(key, msgflg)) < 0) 
    {
        int errnum = errno;
        fprintf(stderr, "Value of errno: %d\n", errno);
        perror("(msgget)");
        fprintf(stderr, "Error msgget: %s\n", strerror( errnum ));
    }
    else
    {
        fprintf(stderr, "msgget: msgget succeeded: msgqid = %d\n", msqid);
    }

    for(int j = 0; j < argc-2; j++)
    {
        //printf("Message(%d): \"%s\" Sent (%d bytes)", j+1, prefixes[j], sizeof(prefixes[j]));

        prefix_buf sbuf;
        sbuf.id = j+1;
        strcpy(sbuf.prefix, prefixes[j]);
        sbuf.mtype = 1;

        size_t buffer_len = strlen(sbuf.prefix) + sizeof(int) + 1;

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
            printf("Message(%d): \"%s\" Sent (%d bytes)\n", sbuf.id, sbuf.prefix,(int)buffer_len);
        }

        int ret;
        response_buf rbuf;
        int responsesRecieved = 0;
        int totalPassages = 1;
        response_buf *responses;
        size_t *responseSizes;
        do 
        {
            ret = msgrcv(msqid, &rbuf, sizeof(response_buf), 2, 0);//receive type 2 message
            int errnum = errno;
            if (ret < 0 && errno !=EINTR)
            {
                fprintf(stderr, "Value of errno: %d\n", errno);
                perror("Error printed by perror");
                fprintf(stderr, "Error receiving msg: %s\n", strerror( errnum ));
            }
            else if(errno != 4)
            {
                responsesRecieved++;
                totalPassages = rbuf.count;

                if(responsesRecieved == 1)
                {
                    responses = (response_buf*)malloc(sizeof(response_buf) * totalPassages);
                    responseSizes = (size_t*)malloc(sizeof(size_t) * totalPassages);
                }

                responses[rbuf.index] = rbuf;
                responseSizes[rbuf.index] = ret;
                /*
                printf("Number of passages: %d of %d\n", responsesRecieved, rbuf.count);
                printf("Word Recieved: %s\n", rbuf.longest_word);
                */
                
            }
        } while (((ret < 0 ) && (errno == 4)) || responsesRecieved < totalPassages);

        for(int k = 0; k < totalPassages; k++)
        {
            if (responses[k].present == 1)
            {
                printf("%d, %d of %d, %s, size=%d\n", responses[k].mtype, responses[k].index, responses[k].count, responses[k].longest_word, responseSizes[k]);
            }
            else
            {
                printf("%d, %d of %d, not found, size=%d\n", responses[k].mtype, responses[k].index, responses[k].count, responseSizes[k]);
            }
                
        }

        //fprintf(stderr,"msgrcv error return code --%d:$d--",ret,errno);
/*
        int totalPassages = rbuf.count;
        int responsesRecieved = 1;

        response_buf responses[totalPassages];
        size_t responseSizes[totalPassages];
        responses[rbuf.index] = rbuf;

        do 
        {
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
                responses[rbuf.index] = rbuf;
                responseSizes[rbuf.index] = ret;
                printf("Longest word recieved: %s, with %d responses recieved of %d\n", rbuf.longest_word, responsesRecieved, totalPassages);
                responsesRecieved++;
            }
        
        } while ((ret < 0 ) && (errno == 4) && responsesRecieved != totalPassages);

        for(int k = 0; k < argc-1; k++)
        {
            if (responses[k].present == 1)
            {
                printf("Longest word: %s\n", responses[k].longest_word );
                //printf("%d, %d of %d, %s, size=%d\n", responses[k].mtype, responses[k].index, responses[k].count, responses[k].longest_word, responseSizes[k]);
            }
            else
            {
                printf("Longest word: %s\n", responses[k].longest_word );
                //printf("%d, %d of %d, not found, size=%d\n", responses[k].mtype, responses[k].index, responses[k].count, responseSizes[k]);
            }
                
        }
    */
        sleep(atoi(argv[1]));
    

    }

    prefix_buf finalSBuf;
    finalSBuf.id = 0;
    strlcpy(finalSBuf.prefix, "", WORD_LENGTH);
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

    printf("Exiting ...");

}