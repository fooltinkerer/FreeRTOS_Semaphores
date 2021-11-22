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
#define TASK1_PRIORITY    ( tskIDLE_PRIORITY + 1 )
#define TASK2_PRIORITY    ( tskIDLE_PRIORITY + 1 )

/* The rate at which data is sent to the queue.  The times are converted from
 * milliseconds to ticks using the pdMS_TO_TICKS() macro. */
#define TASK1_FREQUENCY_MS        pdMS_TO_TICKS( 1000UL )
#define TASK2_FREQUENCY_MS        pdMS_TO_TICKS( 2000UL )
#define A_100_MS_DELAY            pdMS_TO_TICKS( 100UL )


/* The values sent to the queue receive task from the queue send task and the
 * queue send software timer respectively. */
#define TASK1_STACK_SIZE        ( 1000UL )
#define TASK2_STACK_SIZE        ( 1000UL )

/* Activation of semaphore patterns - only one at a time can be active  */
#undef BINARY_SEMAPHORES    ( 1 )
#define COUNTING_SEMAPHORES ( 1 )
#undef MUTEX_PATTERN        ( 1 )
#undef RENDEZ_VOUS_PATTERN  ( 1 )

/* Throw an error in case multiple patterns are active by mistake */
#if defined(COUNTING_SEMAPHORES) && defined(MUTEX_PATTERN) && defined(BINARY_SEMAPHORES)
    #error "Only one pattern at a time can be active! Please reduce this usage to 0 or 1"
#endif

/*-----------------------------------------------------------*/

/*
 * The tasks as described in the comments at the top of this file.
 */
static void prvTask1( void * pvParameters );
static void prvTask2( void * pvParameters );

/* No matter the pattern, the type is always the same */
#if defined(BINARY_SEMAPHORES) || defined(MUTEX_PATTERN) || defined(COUNTING_SEMAPHORES)
static SemaphoreHandle_t mainSemaphore = 0;
#endif

/*-----------------------------------------------------------*/
/* Maximum size of variable*/
#define MAX_STRING_SIZE ( 64UL )

static char task1Text[ ]     = "Little girl with a red hat is walking through the forest";
static char task2Text[ ]     = "A wolf is coming dressed up like it were Carnival";
static char anInitialText[ ] = "What is the name of the story?";
static char aBanner[]        = "**************************************************************"; 
static char printoutText[MAX_STRING_SIZE];

void main_semaphores( void )
{
    /* Initialize */
    memset(&printoutText, 0, sizeof(printoutText));
    strncpy(&printoutText[0], anInitialText, strlen(anInitialText));

#ifdef BINARY_SEMAPHORES
    mainSemaphore = xSemaphoreCreateBinary();
#elif COUNTING_SEMAPHORES
    /* Initialize the semaphore to max_value of 1 and initial value of 1 */
    mainSemaphore = xSemaphoreCreateCounting(1, 1);
#endif    
    if (mainSemaphore == 0) 
    {
        printf("Resouce not created\n");
    }
/* Calling give() is only necessary on binary semaphores as their initial value is 0 */    
#ifdef BINARY_SEMAPHORES
    else
    {
            /* Semaphore needs to be given once so as to make the system work*/  
            xSemaphoreGive(mainSemaphore);
    }
#endif
    /* Print out the initial message */
    printf("%s \n", &aBanner[0]); 
    printf("The initial magic sentence is: %s \n", &printoutText[0]); 
    printf("%s \n", &aBanner[0]); 

    /* Start the two tasks as described in the comments at the top of this
     * file. */
    xTaskCreate( prvTask1           ,             /* The function that implements the task. */
                  "Task1",                         /* The text name assigned to the task - for debug only as it is not used by the kernel. */
                  TASK1_STACK_SIZE,                /* The size of the stack to allocate to the task. */
                  NULL,                            /* The parameter passed to the task - not used in this simple case. */
                  TASK1_PRIORITY, /* The priority assigned to the task. */
                  NULL );                          /* The task handle is not required, so NULL is passed. */

    xTaskCreate( prvTask2           ,             /* The function that implements the task. */
                 "Task2",                         /* The text name assigned to the task - for debug only as it is not used by the kernel. */
                 TASK2_STACK_SIZE,                /* The size of the stack to allocate to the task. */
                 NULL,                            /* The parameter passed to the task - not used in this simple case. */
                 TASK2_PRIORITY, /* The priority assigned to the task. */
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

static void prvTask1(void * pvParameters )
{
     /* Prevent the compiler warning about the unused parameter. */
    ( void ) pvParameters;
    
    /* Local variables*/
    int textLength = strlen(task1Text);

    /* Announce task is ready */   
    printf("\nThis is task 1 - launching\n\n" );

    for( ; ; )
    {
        /* Print out the message */
        /* console_print( "This is task 1\n" ); */

        /* Copy text & printout - let's make sure there is no overrun */
#if defined(BINARY_SEMAPHORES) || defined(COUNTING_SEMAPHORES)
        /* If we can get the semaphore, we change the string */
        if (xSemaphoreTake(mainSemaphore, ( TickType_t ) 0))
        {
            slowStringCopy(&printoutText[0], task1Text, textLength);
            xSemaphoreGive(mainSemaphore);
        }        
#else        
        slowStringCopy(&printoutText[0], task1Text, textLength);
#endif
        printf("The magic sentence is: %s \n", &printoutText[0]);
        fflush(stdout);   
        vTaskDelay(TASK1_FREQUENCY_MS);
    }
}


/*-----------------------------------------------------------*/

static void prvTask2(void * pvParameters )
{
    /* Prevent the compiler warning about the unused parameter. */
    ( void ) pvParameters;
    
    /* Local variables*/
    int textLength = strlen(task2Text);

    /* Announce task is ready */
    printf("\nThis is task 2 - launching\n\n" );

    for( ; ; )
    {
#if defined(BINARY_SEMAPHORES) || defined(COUNTING_SEMAPHORES)
        /* If we can get the semaphore, we change the string */
        if (xSemaphoreTake(mainSemaphore, ( TickType_t ) 0))
        {
            slowStringCopy(&printoutText[0], task2Text, textLength);
            /* As this task is subject to the task as far as printing is concerned  */
            /* let's hold the semaphore a little longer for ensuring our string is  */
            /* visible */
            vTaskDelay(5*TASK1_FREQUENCY_MS);
            xSemaphoreGive(mainSemaphore);
        } 
#else                
        slowStringCopy(&printoutText[0], task2Text, textLength);
#endif        
     
        vTaskDelay(TASK2_FREQUENCY_MS);
    }
}
/*-----------------------------------------------------------*/

void slowStringCopy(char* dest, char* src, int textSize)
{
    int i,max = 0;

    /* Let's reset the content of the destination to '\0' - so no need to set it later*/
    memset(dest, '\0', MAX_STRING_SIZE);
    /* Make sure we do not overflow */
    max = ( textSize < MAX_STRING_SIZE) ? textSize : MAX_STRING_SIZE - 1;
    while (i < max)
    {
        dest[i] =  src[i];
        i++;
        /* Let's make a slow copy */
        for(long long k=0; k<9223372036854775807;k++){};
    }
}
/*-----------------------------------------------------------*/
