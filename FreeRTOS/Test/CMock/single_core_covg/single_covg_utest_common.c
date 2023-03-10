/*
 * FreeRTOS V202112.00
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
/*! @file single_covg_utest_common.c */

#include "single_covg_utest_common.h"

/* C runtime includes. */
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

/* Test includes */
#include "task.h"
#include "global_vars.h"

/* Unity includes. */
#include "unity.h"
#include "unity_memory.h"

/* Mock includes. */
#include "mock_fake_assert.h"
#include "mock_fake_port.h"
#include "mock_timers.h"

/* ===========================  EXTERN VARIABLES  =========================== */

extern List_t pxReadyTasksLists[ configMAX_PRIORITIES ];
extern List_t xDelayedTaskList1;
extern List_t xDelayedTaskList2;
extern volatile UBaseType_t uxDeletedTasksWaitingCleanUp;
extern volatile UBaseType_t uxCurrentNumberOfTasks;
extern volatile TickType_t xTickCount;
extern volatile UBaseType_t uxTopReadyPriority;
extern volatile BaseType_t xSchedulerRunning;
extern volatile TickType_t xPendedTicks;
extern volatile BaseType_t xNumOfOverflows;
extern volatile TickType_t xNextTaskUnblockTime;
extern UBaseType_t uxTaskNumber;
extern TaskHandle_t xIdleTaskHandles[configNUMBER_OF_CORES];
extern volatile UBaseType_t uxSchedulerSuspended;
extern volatile UBaseType_t uxDeletedTasksWaitingCleanUp;
extern List_t * volatile pxDelayedTaskList;
//extern volatile TCB_t *  pxCurrentTCBs[ configNUMBER_OF_CORES ];

static BaseType_t xCoreYields[ configNUMBER_OF_CORES ] = { 0 };

/* portGET_CORE_ID() returns the xCurrentCoreId. The task choose order is dependent on
 * which core calls the FreeRTOS APIs. Setup xCurrentCoreId is required before calling
 * FreeRTOS APIs. The default core is 0. */
static BaseType_t xCurrentCoreId = 0;

/* Each core maintains it's lock count. However, only one core has lock count value > 0.
 * In real world case, this value is read when interrupt disabled while increased when
 * lock is acquired. */
static BaseType_t xIsrLockCount[ configNUMBER_OF_CORES ] = { 0 };
static BaseType_t xTaskLockCount[ configNUMBER_OF_CORES ] = { 0 };

/* ==========================  EXTERN FUNCTIONS  ========================== */

extern void vTaskEnterCritical( void );
extern void vTaskExitCritical( void );

/* ==========================  CALLBACK FUNCTIONS  ========================== */

void * pvPortMalloc( size_t xSize )
{
    return unity_malloc( xSize );
}

void vPortFree( void * pv )
{
    return unity_free( pv );
}

StackType_t * pxPortInitialiseStack( StackType_t * pxTopOfStack,
                                             TaskFunction_t pxCode,
                                             void * pvParameters )
{
    return pxTopOfStack;
}

BaseType_t xPortStartScheduler( void )
{
    uint8_t i;

    /* Initialize each core with a task */
    for (i = 0; i < configNUMBER_OF_CORES; i++) {
        vTaskSwitchContext();
    }

    return pdTRUE;
}

void vPortEndScheduler( void )
{
    return;
}

void vFakePortYieldCoreStubCallback( int xCoreID, int cmock_num_calls )
{
    BaseType_t xCoreInCritical = pdFALSE;
    BaseType_t xPreviousCoreId = xCurrentCoreId;
    int i;

    /* Check if the lock is acquired by any core. */
    for( i = 0; i < configNUMBER_OF_CORES; i++ )
    {
        if( ( xIsrLockCount[ i ] > 0 ) || ( xTaskLockCount[ i ] > 0 ) )
        {
            xCoreInCritical = pdTRUE;
            break;
        }
    }

    if( xCoreInCritical == pdTRUE )
    {
        /* If a is in the critical section, pend the core yield until the
         * task spinlock is released. */
        xCoreYields[ xCoreID ] = pdTRUE;
    } else {
        /* No task is in the critical section. We can yield this core. */
        xCurrentCoreId = xCoreID;
        vTaskSwitchContext();
        xCurrentCoreId = xPreviousCoreId;
    }
}

void vFakePortYieldStubCallback( int cmock_num_calls )
{
    vTaskSwitchContext();
}

void vFakePortEnterCriticalSection( void )
{
    vTaskEnterCritical();
}

/* vTaskExitCritical release the lock then check if this task is requested to yield.
 * The mock implementation assumes all the cores can be requested to yield immediatly.
 * If the core is requested to yield, it will yield in the following order.
 * 1. core ID in accsending order if the core is requested to yield and is not xCurrentCoreId.
 * 2. core ID which is requested to yield and the core ID equals xCurrentCoreId.
 * core i : Core ID requested to yield in critical section in accesending order.
 * ....
 * core xCurrentCoreId : Core ID equals to xCurrentCoreId and is requested to yield in critical section. */
void vFakePortExitCriticalSection( void )
{
    vTaskExitCritical();
}

void vSetCurrentCore( BaseType_t xCoreID )
{
    xCurrentCoreId = xCoreID;
}

static void vYieldCores( void )
{
    BaseType_t i;
    BaseType_t xPreviousCoreId = xCurrentCoreId;

    for( i = 0; i < configNUMBER_OF_CORES; i++ )
    {
        if( xCoreYields[ i ] == pdTRUE )
        {
            xCurrentCoreId = i;
            xCoreYields[ i ] = pdFALSE;
            vTaskSwitchContext();
        }
    }
    xCurrentCoreId = xPreviousCoreId;
}

unsigned int vFakePortGetCoreID( void )
{
    return ( unsigned int )xCurrentCoreId;
}

void vFakePortGetISRLock( void )
{
    int i;

    /* Ensure that no other core is in the critical section. */
    for( i = 0; i < configNUMBER_OF_CORES; i++ )
    {
        if( i != xCurrentCoreId )
        {
            TEST_ASSERT_MESSAGE( xIsrLockCount[ i ] == 0, "vFakePortGetISRLock xIsrLockCount[ i ] > 0" );
            TEST_ASSERT_MESSAGE( xTaskLockCount[ i ] == 0, "vFakePortGetISRLock xTaskLockCount[ i ] > 0" );
        }
    }

    xIsrLockCount[ xCurrentCoreId ]++;
}

void vFakePortReleaseISRLock( void )
{
    TEST_ASSERT_MESSAGE( xIsrLockCount[ xCurrentCoreId ] > 0, "xIsrLockCount[ xCurrentCoreId ] <= 0" );
    xIsrLockCount[ xCurrentCoreId ]--;
}

void vFakePortGetTaskLock( void )
{
    int i;

    /* Ensure that no other core is in the critical section. */
    for( i = 0; i < configNUMBER_OF_CORES; i++ )
    {
        if( i != xCurrentCoreId )
        {
            TEST_ASSERT_MESSAGE( xIsrLockCount[ i ] == 0, "vFakePortGetISRLock xIsrLockCount[ i ] > 0" );
            TEST_ASSERT_MESSAGE( xTaskLockCount[ i ] == 0, "vFakePortGetISRLock xTaskLockCount[ i ] > 0" );
        }
    }

    xTaskLockCount[ xCurrentCoreId ]++;
}

void vFakePortReleaseTaskLock( void )
{
    TEST_ASSERT_MESSAGE( xTaskLockCount[ xCurrentCoreId ] > 0, "xTaskLockCount[ xCurrentCoreId ] <= 0" );
    xTaskLockCount[ xCurrentCoreId ]--;

    /* When releasing the ISR lock, check if any core is waiting to yield. */
    if( xTaskLockCount[ xCurrentCoreId ] == 0 )
    {
        vYieldCores();
    }
}

/* ============================= Unity Fixtures ============================= */

void commonSetUp( void )
{

    vFakePortYieldCore_StubWithCallback( vFakePortYieldCoreStubCallback) ;
    vFakePortYield_StubWithCallback( vFakePortYieldStubCallback );

    vFakeAssert_Ignore();
    vFakePortAssertIfISR_Ignore();
    vFakePortEnableInterrupts_Ignore();

    vFakePortGetTaskLock_Ignore();
    vFakePortGetISRLock_Ignore();
    vFakePortDisableInterrupts_IgnoreAndReturn(1);
    vFakePortRestoreInterrupts_Ignore();
    xTimerCreateTimerTask_IgnoreAndReturn(1);
    vFakePortCheckIfInISR_IgnoreAndReturn(0);
    vPortCurrentTaskDying_Ignore();
    portSetupTCB_CB_Ignore();
    ulFakePortSetInterruptMask_IgnoreAndReturn(0);
    vFakePortClearInterruptMask_Ignore();

    memset( &pxReadyTasksLists, 0x00, configMAX_PRIORITIES * sizeof( List_t ) );
    memset( &xDelayedTaskList1, 0x00, sizeof( List_t ) );
    memset( &xDelayedTaskList2, 0x00, sizeof( List_t ) );
    memset( &xIdleTaskHandles, 0x00, (configNUMBER_OF_CORES * sizeof( TaskHandle_t )) );
    //memset( &pxCurrentTCBs, 0x00, (configNUMBER_OF_CORES * sizeof( TCB_t * )) );

    uxDeletedTasksWaitingCleanUp = 0;
    uxCurrentNumberOfTasks = ( UBaseType_t ) 0U;
    xTickCount = ( TickType_t ) 500; /* configINITIAL_TICK_COUNT */
    uxTopReadyPriority = tskIDLE_PRIORITY;
    xSchedulerRunning = pdFALSE;
    xPendedTicks = ( TickType_t ) 0U;
    xNumOfOverflows = ( BaseType_t ) 0;
    uxTaskNumber = ( UBaseType_t ) 0U;
    xNextTaskUnblockTime = ( TickType_t ) 0U;
    uxSchedulerSuspended = ( UBaseType_t ) 0;
    uxDeletedTasksWaitingCleanUp = 0;
    pxDelayedTaskList = NULL;

    xCurrentCoreId = 0;
    memset( xTaskLockCount, 0x00, sizeof( xTaskLockCount ) );
    memset( xIsrLockCount, 0x00, sizeof( xIsrLockCount ) );
}

void commonTearDown( void )
{

}

/* ==========================  Helper functions =========================== */

void vSmpTestTask( void *pvParameters )
{
}

