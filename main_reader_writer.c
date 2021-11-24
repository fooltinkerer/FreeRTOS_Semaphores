/*
 * FreeRTOS V202111.00
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * https://www.FreeRTOS.org
 * https://github.com/FreeRTOS
 *
 */

/******************************************************************************
 * NOTE 1: The FreeRTOS demo threads will not be running continuously, so
 * do not expect to get real time behaviour from the FreeRTOS Linux port, or
 * this demo application.  Also, the timing information in the FreeRTOS+Trace
 * logs have no meaningful units.  See the documentation page for the Linux
 * port for further information:
 * https://freertos.org/FreeRTOS-simulator-for-Linux.html
 *
 * NOTE:  Console input and output relies on Linux system calls, which can
 * interfere with the execution of the FreeRTOS Linux port. This demo only
 * uses Linux system call occasionally. Heavier use of Linux system calls
 * may crash the port.
 */

#include <stdio.h>
#include <pthread.h>
#include <string.h>

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "semphr.h"
#include "assert.h"
#include "task.h"

/* Local includes. */
#include "console.h"

/* Priorities at which the tasks are created. */
#define READER_PRIORITY    ( tskIDLE_PRIORITY + 1 )
#define WRITER_PRIORITY    ( tskIDLE_PRIORITY + 1 )

/* The rate at which data is sent to the queue.  The times are converted from
 * milliseconds to ticks using the pdMS_TO_TICKS() macro. */
#define READER_FREQUENCY_MS       pdMS_TO_TICKS( 10000UL )
#define WRITER_FREQUENCY_MS       pdMS_TO_TICKS( 20000UL )
#define A_100_MS_DELAY            pdMS_TO_TICKS( 100UL )


/* The values sent to the queue receive task from the queue send task and the
 * queue send software timer respectively. */
#define READER_STACK_SIZE        ( 1000UL )
#define WRITER_STACK_SIZE        ( 1000UL )

/* Activation of semaphore patterns - only one at a time can be active  */
#undef PRIORITIZED_WRITER 

/*-----------------------------------------------------------*/

/*
 * The tasks as described in the comments at the top of this file.
 */
static void prvReader( void * pvParameters );
static void prvWriter( void * pvParameters );


/* No matter the pattern, the type is always the same */
static SemaphoreHandle_t newsSpace = 0;
static SemaphoreHandle_t mutex     = 0;

/*-----------------------------------------------------------*/
/* Maximum size of variable*/
#define MAX_STRING_SIZE ( 64UL )


/*-----------------------------------------------------------*/

void changeContentOfNewspaper(void)
{
    /* Let's reset the content of the destination to '\0' - so no need to set it later*/
    printf("\t\tWriter just changed the content\n");
}
/*-----------------------------------------------------------*/

void main_readers_writer( void )
{
    /* Initialize */
    int readerNr[4];
    
    newsSpace = xSemaphoreCreateMutex();
    mutex     = xSemaphoreCreateMutex();
    
    /* Start the reader tasks as described in the comments at the top of this
     * file. */
    for (int i = 0; i <= 3; i++){
        char buffer[MAX_STRING_SIZE];
        memset(buffer, '\0', MAX_STRING_SIZE);
        sprintf(buffer, "%s%d", "Reader", i);

        readerNr[i] = i;
        xTaskCreate(prvReader           ,                    /* The function that implements the task. */
                    buffer, /* The text name assigned to the task - for debug only as it is not used by the kernel. */
                    READER_STACK_SIZE,                          /* The size of the stack to allocate to the task. */
                    (void *) &readerNr[i],                                       /* The parameter passed to the task - not used in this simple case. */
                    READER_PRIORITY,                            /* The priority assigned to the task. */
                    NULL );                                     /* The task handle is not required, so NULL is passed. */
    }

    xTaskCreate( prvWriter           ,             /* The function that implements the task. */
                 "Writer",                         /* The text name assigned to the task - for debug only as it is not used by the kernel. */
                 WRITER_STACK_SIZE,                /* The size of the stack to allocate to the task. */
                 NULL,                            /* The parameter passed to the task - not used in this simple case. */
                 WRITER_PRIORITY, /* The priority assigned to the task. */
                 NULL );                          /* The task handle is not required, so NULL is passed. */

    /* Start the tasks and timer running. */
    vTaskStartScheduler();  

    /* If all is well, the scheduler will now be running, and the following
     * line will never be reached.  If the following line does execute, then
     * there was insufficient FreeRTOS heap memory available for the idle and/or
     * timer tasks	to be created.  See the memory management section on the
     * FreeRTOS web site for more details. */
    for( ; ; )
    {
    }
}

/*-----------------------------------------------------------*/

static void prvReader(void * pvParameters )
{
    /* Prevent the compiler warning about the unused parameter. */
    int delayMultiplier = *((int*) pvParameters);
    int readers = 0;

    for( ; ; )
    {
        /* Try to get to read the news or just go to sleep */
        if (xSemaphoreTake(mutex, ( TickType_t ) 0)){
            readers++;
            /* IF the first reader, wait indefinitely until the writer is through */
            if (1 == readers) xSemaphoreTake(newsSpace, portMAX_DELAY);
            xSemaphoreGive(mutex);

            printf("The %s is reading the paper\n", pcTaskGetName(xTaskGetCurrentTaskHandle()));

            /* We are done reading, let's return*/
            if (xSemaphoreTake(mutex, portMAX_DELAY)){
                readers--;
                /* We are the last reader, let's return the newsSpace */
                if (0 == readers) xSemaphoreGive(newsSpace);
                xSemaphoreGive(mutex);
            }
        }

        vTaskDelay(delayMultiplier * 50 * A_100_MS_DELAY+ READER_FREQUENCY_MS);
    }
}


/*-----------------------------------------------------------*/

static void prvWriter(void * pvParameters )
{
    /* Prevent the compiler warning about the unused parameter. */
    ( void ) pvParameters;
    
    /* Local variables*/

    for( ; ; )
    {
        if (xSemaphoreTake(newsSpace, ( TickType_t ) 0)){
            changeContentOfNewspaper();
            xSemaphoreGive(newsSpace);
        }

        vTaskDelay(WRITER_FREQUENCY_MS);
    }
}
/*-----------------------------------------------------------*/

