/* The default action for libdesock is to
 * read from stdin / write to stdout
 * but this behaviour can be changed with the
 * following to functions.
 * They have to behave like glibc functions:
 * On success they must return a value >= 0 indicating
 * how many bytes have been read / written.
 * On failure they must return -1 with errno set to the
 * corresponding error.
 *
 * Added support for reading multiple inputs from a file
 *     Kelly Patterson - Cisco Talos
 *     Copyright (C) 2024 Cisco Systems Inc
 *
 */
#ifdef MULTI_REQUEST
#include <limits.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include "desock.h"
#endif
#include "hooks.h"
#include "syscall.h"
 

#ifdef MULTI_REQUEST
char* delimiter = "=^..^=";

/* This function will "look ahead" into the file by reading the length of the remaining 
    bytes of the delimiter and check if those next bytes match the delimiter. This gets 
    called if there is a potential of the delimiter matching at the boundary of the read 
    buffer length */
bool peekDelimiter(uint delimIx, size_t delimLen)
{
    int currPos = 0;
    char* peekBuf = NULL;
    int peekLen = 0;
    ssize_t readLen = 0;
    int peekIdx = 0;

    if (delimLen <= delimIx)
    {
        return false;
    }

    peekLen = delimLen-delimIx;

    currPos = lseek(0, 0, SEEK_CUR);
    if (-1 == currPos)
    {
        perror("lseek");
        return false;
    }

    peekBuf = calloc(peekLen, sizeof(char));

    if (NULL == peekBuf)
    {
        return false;
    }

    readLen = syscall_cp(SYS_read, 0, peekBuf, peekLen);

    if (readLen != peekLen)
    {
        /* Reset the file pointer */
        lseek(0, currPos,SEEK_SET);
        free(peekBuf);
        return false;
    }

    while (delimIx < delimLen && peekIdx < peekLen)
    {
        if (peekBuf[peekIdx] == delimiter[delimIx])
        {
            peekIdx++;
            delimIx++;
        }
        else
        {
            /* Reset the file pointer */
            lseek(0, currPos,SEEK_SET);
            free(peekBuf);
            return false;
        }
    }

    /* Reset the file pointer */
    lseek(0, currPos,SEEK_SET);
    free(peekBuf);
    return true;

}
ssize_t findDelimiterOffset(char* buf, size_t bufLen, char* delim, size_t delimLen)
{
    uint bufIdx = 0;
    uint delimIdx = 0;
    bool found=false;

    if (delimLen <= 0)
    {
        return -1;
    }

    while (!found && bufIdx<bufLen && delimIdx< delimLen)
    {
        if (buf[bufIdx] == delim[delimIdx])
        {
            bufIdx++;
            delimIdx++;
            while (!found && bufIdx<bufLen && delimIdx< delimLen)
            {
                if (buf[bufIdx] == delim[delimIdx])
                {
                    bufIdx++;
                    delimIdx++;
                }
                else
                {
                    delimIdx = 0;
                    break;
                }
            }
            if (delimIdx == delimLen)
            {
                found = true;
                break;
            }

        }
        bufIdx++;
    }
    if (found)
    {
        return bufIdx-delimLen;
    }
    else if(delimIdx != 0 && delimIdx != delimLen)
    {
        /* We might have a match on the boundary of bufLen*/
        /* Getting here means that at least 1 byte of the delimiter matched
            the last byte(s) in buf */
        if (peekDelimiter(delimIdx, delimLen))
        {
            /* where in the buffer the delimiter began */
            return bufLen-delimIdx;
        }
    }
    /* not found */
    return -1;
}
ssize_t readOneChunk(char* buf, size_t size)
{
    char* tmpBuf = NULL;
    int currPos = 0;
    ssize_t delimiterOffset = 0;
    ssize_t readLen = 0;

    tmpBuf = calloc(size, sizeof(char));

    if (NULL == tmpBuf)
    {
        return -1;
    }

    currPos = lseek(0, 0, SEEK_CUR);
    if (-1 == currPos)
    {
        perror("lseek");
        free(tmpBuf);
        return -1;
    }

    readLen = syscall_cp(SYS_read, 0, tmpBuf, size);

    if (readLen < 0)
    {
        return -1;
    }

    delimiterOffset = findDelimiterOffset(tmpBuf, size, delimiter, strlen(delimiter));

    if (delimiterOffset > 0 && size >= (size_t)delimiterOffset)
    {
        DEBUG_LOG ("[%d] desock::readOneChunk current file pos (0x%08x) delimiterOffset (0x%08x)\n", gettid (), currPos, delimiterOffset);
        /* copy the buffer up to the delimiter*/
        memcpy(buf, tmpBuf, (size_t)delimiterOffset);
        /* set the file pointer to after the delimiter*/
        if (-1 == lseek(0, currPos+delimiterOffset+strlen(delimiter),SEEK_SET))
        {
            free(tmpBuf);
            return -1;
        }
        free(tmpBuf);
        return delimiterOffset;
    }
    else
    {   
        DEBUG_LOG ("desock::readOneChunk delimiter not found \n");
        /* delimiter not found, copy the whole buffer*/
        memcpy(buf, tmpBuf, readLen);
    }

    free(tmpBuf);
    return readLen;
}
#endif
/* This function is called whenever a read on a network
 * connection occurs. Read from stdin instead.
 */
ssize_t hook_input (char* buf, size_t size) {
    #ifdef MULTI_REQUEST
    /* read one chunk*/
    ssize_t ret = 0;
    ret = readOneChunk(buf, size);
    if (-1 != ret)
    {
        return ret;
    }
    #endif 
    return syscall_cp(SYS_read, 0, buf, size);
}

/* This function is called whenever a write on a network
 * connection occurs. Write to stdout instead.
 */
ssize_t hook_output (char* buf, size_t size) {
#ifdef DEBUG
    return syscall_cp(SYS_write, 1, buf, size);
#else
    return (ssize_t) size;
#endif
}
