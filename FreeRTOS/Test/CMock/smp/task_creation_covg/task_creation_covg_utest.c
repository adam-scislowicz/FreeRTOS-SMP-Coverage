/*
 * FreeRTOS V202012.00
 * Copyright (C) 2022 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
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
/*! @file task_creation_covg_utest */

/* C runtime includes. */
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/* Task includes */
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "event_groups.h"
#include "queue.h"

/* Test includes. */
#include "unity.h"
#include "unity_memory.h"
#include "../global_vars.h"
#include "../smp_utest_common.h"

/* Mock includes. */
#include "mock_timers.h"
#include "mock_fake_assert.h"
#include "mock_fake_port.h"

#define taskTASK_YIELDING       ( TaskRunning_t ) ( -2 )


/* ===========================  EXTERN VARIABLES  =========================== */
extern volatile UBaseType_t uxDeletedTasksWaitingCleanUp;
extern volatile UBaseType_t uxSchedulerSuspended;
extern volatile TCB_t *  pxCurrentTCBs[ configNUMBER_OF_CORES ];
extern volatile BaseType_t xSchedulerRunning;
extern volatile TickType_t xTickCount;
extern volatile UBaseType_t uxCurrentNumberOfTasks;

/* ==============================  Global VARIABLES ============================== */
TaskHandle_t xTaskHandles[configNUMBER_OF_CORES] = { NULL };

/* ============================  Unity Fixtures  ============================ */
/*! called before each testcase */
void setUp( void )
{
    commonSetUp();
}

/*! called after each testcase */
void tearDown( void )
{
    commonTearDown();
}

/*! called at the beginning of the whole suite */
void suiteSetUp()
{
}

/*! called at the end of the whole suite */
int suiteTearDown( int numFailures )
{
    return numFailures;
}

/* ===========================  EXTERN FUNCTIONS  =========================== */
extern void vTaskEnterCritical(void);

/* ==============================  Helper functions for Test Cases  ============================== */


/* ==============================  Test Cases  ============================== */

/*
The kernel will be configured as follows:
    #define configNUMBER_OF_CORES                           (N > 1)
    #define configUSE_CORE_AFFINITY                         1
    #define configUSE_TASK_PREEMPTION_DISABLE               1

Coverage for 
        static void prvAddNewTaskToReadyList( TCB_t * pxNewTCB )
        covers the case where the task being created is not the first or only task.
*/
void test_create_two_tasks_with_the_first_suspended( void )
{
    TaskHandle_t xTaskHandles[configNUMBER_OF_CORES] = { NULL };

    xTaskCreate( vSmpTestTask, "SMP Task", configMINIMAL_STACK_SIZE, NULL, 1, &xTaskHandles[0] );
    vTaskSuspend(xTaskHandles[0]);

    xTaskCreate( vSmpTestTask, "SMP Task", configMINIMAL_STACK_SIZE, NULL, 1, &xTaskHandles[1] );

    vTaskStartScheduler();
}

/*
The kernel will be configured as follows:
    #define configNUMBER_OF_CORES                           (N > 1)
    #define configUSE_CORE_AFFINITY                         1
    #define configUSE_TASK_PREEMPTION_DISABLE               1

Coverage for 
        static void prvAddNewTaskToReadyList( TCB_t * pxNewTCB )
        covers the case where there coreID is out of bounds when looking for a TCB
*/

void vSmpForeverTestTask( void *pvParameters )
{
    for(;;)
    {
    }
}

void show_task_status( void )
{
    UBaseType_t uxIdx;

    for(uxIdx=0; uxIdx < configNUMBER_OF_CORES; uxIdx++)
    {
        if (pxCurrentTCBs[uxIdx] != NULL)
        {
            printf("    [%d]: 0x%lX\n", (int)uxIdx, pxCurrentTCBs[uxIdx]->uxTaskAttributes);
        }
    }
}

void test_create_more_tasks_than_there_are_cores( void )
{
    TaskHandle_t xTaskHandles[configNUMBER_OF_CORES+3] = { NULL };

    UBaseType_t uxTaskNum;

    for(uxTaskNum=0; uxTaskNum <= configNUMBER_OF_CORES+1; uxTaskNum++)
    {
        xTaskCreate( vSmpTestTask, "SMP Task", configMINIMAL_STACK_SIZE, NULL, 1, &xTaskHandles[uxTaskNum]);
        printf("uxCurrentNumberOfTasks: %d, uxTaskNum=%d\n", (int)uxCurrentNumberOfTasks, (int)uxTaskNum);
        show_task_status();
        vTaskPreemptionDisable( xTaskHandles[uxTaskNum] );
    }

    //TEST_ASSERT_EQUAL( configNUMBER_OF_CORES, uxCurrentNumberOfTasks );
    printf("ALL TASKS RUNNING:\n");
    printf("uxCurrentNumberOfTasks: %d, uxTaskNum=%d\n", (int)uxCurrentNumberOfTasks, (int)configNUMBER_OF_CORES+2);
    show_task_status();
    xTaskCreate( vSmpTestTask, "SMP Task", configMINIMAL_STACK_SIZE, NULL, 1, &xTaskHandles[configNUMBER_OF_CORES+2]);
    printf("xTaskHandles[%d]: 0x%lX\n", (int)configNUMBER_OF_CORES+2, xTaskHandles[configNUMBER_OF_CORES+2]->uxTaskAttributes);
    
    vTaskStartScheduler();
    
    show_task_status();
}
