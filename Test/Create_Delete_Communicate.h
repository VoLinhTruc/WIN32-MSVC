/* Standard includes. */
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>

/* Visual studio intrinsics used so the __debugbreak() function is available
should an assert get hit. */
#include <intrin.h>

/* FreeRTOS kernel includes. */
#include "FreeRTOS.h"
#include "task.h"

#include "Tree_Back_Model.h"
#include "Tree_Back_Managing_Task.h"
#include "Tree_Back_Executing_Task.h"

#include "Bico_MQTT_Utility.h"
#include "Bico_MQTT_Packet_Type.h"
#include "Bico_MQTT_Packet_Quick_Init.h"
#include "Bico_MQTT_Packet_Serialize.h"
#include "Bico_MQTT_Packet_Deserialize.h"


#define mainCREATE_SIMPLE_BLINKY_DEMO_ONLY	1

/* This demo uses heap_5.c, and these constants define the sizes of the regions
that make up the total heap.  heap_5 is only used for test and example purposes
as this demo could easily create one large heap region instead of multiple
smaller heap regions - in which case heap_4.c would be the more appropriate
choice.  See http://www.freertos.org/a00111.html for an explanation. */
#define mainREGION_1_SIZE	(8201 + 1024*50)
#define mainREGION_2_SIZE	(29905 + 1024*50)
#define mainREGION_3_SIZE	(7607 + 1024*50)

/*-----------------------------------------------------------*/

/*
 * main_blinky() is used when mainCREATE_SIMPLE_BLINKY_DEMO_ONLY is set to 1.
 * main_full() is used when mainCREATE_SIMPLE_BLINKY_DEMO_ONLY is set to 0.
 */
extern void main_blinky(void);
extern void main_full(void);

/*
 * Only the comprehensive demo uses application hook (callback) functions.  See
 * http://www.freertos.org/a00016.html for more information.
 */
void vFullDemoTickHookFunction(void);
void vFullDemoIdleFunction(void);

/*
 * This demo uses heap_5.c, so start by defining some heap regions.  It is not
 * necessary for this demo to use heap_5, as it could define one large heap
 * region.  Heap_5 is only used for test and example purposes.  See
 * http://www.freertos.org/a00111.html for an explanation.
 */
static void  prvInitialiseHeap(void);

/*
 * Prototypes for the standard FreeRTOS application hook (callback) functions
 * implemented within this file.  See http://www.freertos.org/a00016.html .
 */
void vApplicationMallocFailedHook(void);
void vApplicationIdleHook(void);
void vApplicationStackOverflowHook(TaskHandle_t pxTask, char* pcTaskName);
void vApplicationTickHook(void);
void vApplicationGetIdleTaskMemory(StaticTask_t** ppxIdleTaskTCBBuffer, StackType_t** ppxIdleTaskStackBuffer, uint32_t* pulIdleTaskStackSize);
void vApplicationGetTimerTaskMemory(StaticTask_t** ppxTimerTaskTCBBuffer, StackType_t** ppxTimerTaskStackBuffer, uint32_t* pulTimerTaskStackSize);

/*
 * Writes trace data to a disk file when the trace recording is stopped.
 * This function will simply overwrite any trace files that already exist.
 */
static void prvSaveTraceFile(void);

/*-----------------------------------------------------------*/

/* When configSUPPORT_STATIC_ALLOCATION is set to 1 the application writer can
use a callback function to optionally provide the memory required by the idle
and timer tasks.  This is the stack that will be used by the timer task.  It is
declared here, as a global, so it can be checked by a test that is implemented
in a different file. */
StackType_t uxTimerTaskStack[configTIMER_TASK_STACK_DEPTH];

/* Notes if the trace is running or not. */
static BaseType_t xTraceRunning = pdTRUE;

/*-----------------------------------------------------------*/

void task0(void* pvParameters);
void task1(void* pvParameters);

int main(void)
{
	/* This demo uses heap_5.c, so start by defining some heap regions.  heap_5
	is only used for test and example reasons.  Heap_4 is more appropriate.  See
	http://www.freertos.org/a00111.html for an explanation. */
	prvInitialiseHeap();

	/* Initialise the trace recorder.  Use of the trace recorder is optional.
	See http://www.FreeRTOS.org/trace for more information. */
	vTraceEnable(TRC_START);

	TB_Task_Handle task0_a_handle = initTBTask();
	createTBTask(NULL, 3, sizeof(TB_MQ_Handle), NULL, 0, 0, task0, "task0_a", configMINIMAL_STACK_SIZE, tskIDLE_PRIORITY, &task0_a_handle);

	TB_Task_Handle task0_b_handle = initTBTask();
	createTBTask(NULL, 3, sizeof(TB_MQ_Handle), NULL, 0, 0, task0, "task0_b", configMINIMAL_STACK_SIZE, tskIDLE_PRIORITY, &task0_b_handle);
	
	TB_Task_Handle tb_task_handle_1 = initTBTask();
	createTBTask(NULL, 1, 1, NULL, 0, 0, task1, "task1", configMINIMAL_STACK_SIZE, tskIDLE_PRIORITY, &tb_task_handle_1);
	
	vTaskStartScheduler();

	return 0;
}
/*-----------------------------------------------------------*/


void task0(void* pvParameters)
{
	// Get Tree-Back-Task Handle
	TB_Task_Handle tb_task = (TB_Task_Handle)pvParameters;


	// Setup - begin --------------------------------------------------------------------

	// Setup - end --------------------------------------------------------------------

	// Infnite loop
	while (1)
	{

		// Normal run - begin --------------------------------------------------------------------
		printf("%s\r\n", getTBTaskName(tb_task));

		// Get incoming data - begin --------------------------------------------------
		char* mess = (char*)pvPortMalloc(20);
		char* data = (char*)pvPortMalloc(20);
		char* src = (char*)pvPortMalloc(20);
		if (quickDequeueTBMQ(tb_task->qin, mess, data, src) == 1)
		{
			printf("%s received %s, %s from %s\r\n", getTBTaskName(tb_task), mess, data, src);
		}
		vPortFree(mess);
		vPortFree(data);
		vPortFree(src);
		// Get incoming data - end --------------------------------------------------
		// Normal run - end --------------------------------------------------------------------


		// Delay
		vTaskDelay(500);
	}
}

void task1(void* pvParameters)
{
	// Get Tree-Back-Task Handle
	TB_Task_Handle tb_task = (TB_Task_Handle)pvParameters;


	// Setup - begin --------------------------------------------------------------------
	uint32_t count = 0;
	// Setup - end --------------------------------------------------------------------

	// Infnite loop
	while (count < 5)
	{

		// Normal run - begin --------------------------------------------------------------------
		printf("%s\r\n", getTBTaskName(tb_task));

		// Find specific task then send mess_data to that task
		TB_Task_Table_Handle found_task = initTBTaskTable();
		if (getTBTaskByName("task0_a", &found_task) == 1)
		{
			quickEnqueueTBMQ(found_task->tb_task_handle->qin, "xxx", "123", getTBTaskName(tb_task));
		}
		if (getTBTaskByName("task0_b", &found_task) == 1)
		{
			quickEnqueueTBMQ(found_task->tb_task_handle->qin, "yyy", "456", getTBTaskName(tb_task));
		}

		// Normal run - end --------------------------------------------------------------------

		// Delay
		vTaskDelay(2000);

		count++;
	}

	deleteTBTask(&tb_task);
}

/*-----------------------------------------------------------*/

void vApplicationMallocFailedHook(void)
{
	/* vApplicationMallocFailedHook() will only be called if
	configUSE_MALLOC_FAILED_HOOK is set to 1 in FreeRTOSConfig.h.  It is a hook
	function that will get called if a call to pvPortMalloc() fails.
	pvPortMalloc() is called internally by the kernel whenever a task, queue,
	timer or semaphore is created.  It is also called by various parts of the
	demo application.  If heap_1.c, heap_2.c or heap_4.c is being used, then the
	size of the	heap available to pvPortMalloc() is defined by
	configTOTAL_HEAP_SIZE in FreeRTOSConfig.h, and the xPortGetFreeHeapSize()
	API function can be used to query the size of free heap space that remains
	(although it does not provide information on how the remaining heap might be
	fragmented).  See http://www.freertos.org/a00111.html for more
	information. */
	vAssertCalled(__LINE__, __FILE__);
}
/*-----------------------------------------------------------*/

void vApplicationIdleHook(void)
{
	/* vApplicationIdleHook() will only be called if configUSE_IDLE_HOOK is set
	to 1 in FreeRTOSConfig.h.  It will be called on each iteration of the idle
	task.  It is essential that code added to this hook function never attempts
	to block in any way (for example, call xQueueReceive() with a block time
	specified, or call vTaskDelay()).  If application tasks make use of the
	vTaskDelete() API function to delete themselves then it is also important
	that vApplicationIdleHook() is permitted to return to its calling function,
	because it is the responsibility of the idle task to clean up memory
	allocated by the kernel to any task that has since deleted itself. */

	/* Uncomment the following code to allow the trace to be stopped with any
	key press.  The code is commented out by default as the kbhit() function
	interferes with the run time behaviour. */
	/*
		if( _kbhit() != pdFALSE )
		{
			if( xTraceRunning == pdTRUE )
			{
				vTraceStop();
				prvSaveTraceFile();
				xTraceRunning = pdFALSE;
			}
		}
	*/

#if ( mainCREATE_SIMPLE_BLINKY_DEMO_ONLY != 1 )
	{
		/* Call the idle task processing used by the full demo.  The simple
		blinky demo does not use the idle task hook. */
		vFullDemoIdleFunction();
	}
#endif
}
/*-----------------------------------------------------------*/

void vApplicationStackOverflowHook(TaskHandle_t pxTask, char* pcTaskName)
{
	(void)pcTaskName;
	(void)pxTask;

	/* Run time stack overflow checking is performed if
	configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
	function is called if a stack overflow is detected.  This function is
	provided as an example only as stack overflow checking does not function
	when running the FreeRTOS Windows port. */
	vAssertCalled(__LINE__, __FILE__);
}
/*-----------------------------------------------------------*/

void vApplicationTickHook(void)
{
	/* This function will be called by each tick interrupt if
	configUSE_TICK_HOOK is set to 1 in FreeRTOSConfig.h.  User code can be
	added here, but the tick hook is called from an interrupt context, so
	code must not attempt to block, and only the interrupt safe FreeRTOS API
	functions can be used (those that end in FromISR()). */

#if ( mainCREATE_SIMPLE_BLINKY_DEMO_ONLY != 1 )
	{
		vFullDemoTickHookFunction();
	}
#endif /* mainCREATE_SIMPLE_BLINKY_DEMO_ONLY */
}
/*-----------------------------------------------------------*/

void vApplicationDaemonTaskStartupHook(void)
{
	/* This function will be called once only, when the daemon task starts to
	execute	(sometimes called the timer task).  This is useful if the
	application includes initialisation code that would benefit from executing
	after the scheduler has been started. */
}
/*-----------------------------------------------------------*/

void vAssertCalled(unsigned long ulLine, const char* const pcFileName)
{
	static BaseType_t xPrinted = pdFALSE;
	volatile uint32_t ulSetToNonZeroInDebuggerToContinue = 0;

	/* Called if an assertion passed to configASSERT() fails.  See
	http://www.freertos.org/a00110.html#configASSERT for more information. */

	/* Parameters are not used. */
	(void)ulLine;
	(void)pcFileName;

	printf("ASSERT! Line %ld, file %s, GetLastError() %ld\r\n", ulLine, pcFileName, GetLastError());

	taskENTER_CRITICAL();
	{
		/* Stop the trace recording. */
		if (xPrinted == pdFALSE)
		{
			xPrinted = pdTRUE;
			if (xTraceRunning == pdTRUE)
			{
				vTraceStop();
				prvSaveTraceFile();
			}
		}

		/* Cause debugger break point if being debugged. */
		__debugbreak();

		/* You can step out of this function to debug the assertion by using
		the debugger to set ulSetToNonZeroInDebuggerToContinue to a non-zero
		value. */
		while (ulSetToNonZeroInDebuggerToContinue == 0)
		{
			__asm { NOP };
			__asm { NOP };
		}
	}
	taskEXIT_CRITICAL();
}
/*-----------------------------------------------------------*/

static void prvSaveTraceFile(void)
{
	FILE* pxOutputFile;

	fopen_s(&pxOutputFile, "Trace.dump", "wb");

	if (pxOutputFile != NULL)
	{
		fwrite(RecorderDataPtr, sizeof(RecorderDataType), 1, pxOutputFile);
		fclose(pxOutputFile);
		printf("\r\nTrace output saved to Trace.dump\r\n");
	}
	else
	{
		printf("\r\nFailed to create trace dump file\r\n");
	}
}
/*-----------------------------------------------------------*/

static void  prvInitialiseHeap(void)
{
	/* The Windows demo could create one large heap region, in which case it would
	be appropriate to use heap_4.  However, purely for demonstration purposes,
	heap_5 is used instead, so start by defining some heap regions.  No
	initialisation is required when any other heap implementation is used.  See
	http://www.freertos.org/a00111.html for more information.

	The xHeapRegions structure requires the regions to be defined in start address
	order, so this just creates one big array, then populates the structure with
	offsets into the array - with gaps in between and messy alignment just for test
	purposes. */
	static uint8_t ucHeap[configTOTAL_HEAP_SIZE];
	volatile uint32_t ulAdditionalOffset = 19; /* Just to prevent 'condition is always true' warnings in configASSERT(). */
	const HeapRegion_t xHeapRegions[] =
	{
		/* Start address with dummy offsets						Size */
		{ ucHeap + 1,											mainREGION_1_SIZE },
		{ ucHeap + 15 + mainREGION_1_SIZE,						mainREGION_2_SIZE },
		{ ucHeap + 19 + mainREGION_1_SIZE + mainREGION_2_SIZE,	mainREGION_3_SIZE },
		{ NULL, 0 }
	};

	/* Sanity check that the sizes and offsets defined actually fit into the
	array. */
	configASSERT((ulAdditionalOffset + mainREGION_1_SIZE + mainREGION_2_SIZE + mainREGION_3_SIZE) < configTOTAL_HEAP_SIZE);

	/* Prevent compiler warnings when configASSERT() is not defined. */
	(void)ulAdditionalOffset;

	vPortDefineHeapRegions(xHeapRegions);
}
/*-----------------------------------------------------------*/

/* configUSE_STATIC_ALLOCATION is set to 1, so the application must provide an
implementation of vApplicationGetIdleTaskMemory() to provide the memory that is
used by the Idle task. */
void vApplicationGetIdleTaskMemory(StaticTask_t** ppxIdleTaskTCBBuffer, StackType_t** ppxIdleTaskStackBuffer, uint32_t* pulIdleTaskStackSize)
{
	/* If the buffers to be provided to the Idle task are declared inside this
	function then they must be declared static - otherwise they will be allocated on
	the stack and so not exists after this function exits. */
	static StaticTask_t xIdleTaskTCB;
	static StackType_t uxIdleTaskStack[configMINIMAL_STACK_SIZE];

	/* Pass out a pointer to the StaticTask_t structure in which the Idle task's
	state will be stored. */
	*ppxIdleTaskTCBBuffer = &xIdleTaskTCB;

	/* Pass out the array that will be used as the Idle task's stack. */
	*ppxIdleTaskStackBuffer = uxIdleTaskStack;

	/* Pass out the size of the array pointed to by *ppxIdleTaskStackBuffer.
	Note that, as the array is necessarily of type StackType_t,
	configMINIMAL_STACK_SIZE is specified in words, not bytes. */
	*pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}
/*-----------------------------------------------------------*/

/* configUSE_STATIC_ALLOCATION and configUSE_TIMERS are both set to 1, so the
application must provide an implementation of vApplicationGetTimerTaskMemory()
to provide the memory that is used by the Timer service task. */
void vApplicationGetTimerTaskMemory(StaticTask_t** ppxTimerTaskTCBBuffer, StackType_t** ppxTimerTaskStackBuffer, uint32_t* pulTimerTaskStackSize)
{
	/* If the buffers to be provided to the Timer task are declared inside this
	function then they must be declared static - otherwise they will be allocated on
	the stack and so not exists after this function exits. */
	static StaticTask_t xTimerTaskTCB;

	/* Pass out a pointer to the StaticTask_t structure in which the Timer
	task's state will be stored. */
	*ppxTimerTaskTCBBuffer = &xTimerTaskTCB;

	/* Pass out the array that will be used as the Timer task's stack. */
	*ppxTimerTaskStackBuffer = uxTimerTaskStack;

	/* Pass out the size of the array pointed to by *ppxTimerTaskStackBuffer.
	Note that, as the array is necessarily of type StackType_t,
	configMINIMAL_STACK_SIZE is specified in words, not bytes. */
	*pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}

