#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* Global Variables */

QueueHandle_t telemetryQueue;

TaskHandle_t telemetryHandle;
TaskHandle_t loggingHandle;

int currentPhase = 1;

/* ---------------- TELEMETRY TASK ---------------- */

void TelemetryTask(void *pvParameters)
{
    int data = 0;

    while (1)
    {
        data++;

        printf("Telemetry Sent: %d\n", data);

        xQueueSend(telemetryQueue, &data, 0);

        if (currentPhase == 1)      // Launch
        {
            vTaskDelay(pdMS_TO_TICKS(500));
        }
        else if (currentPhase == 2) // Ascent
        {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
        else                        // Orbit
        {
            vTaskDelay(pdMS_TO_TICKS(2000));
        }
    }
}

/* ---------------- LOGGING TASK ---------------- */

void LoggingTask(void *pvParameters)
{
    int receivedData;

    while (1)
    {
        if (xQueueReceive(telemetryQueue,
                          &receivedData,
                          portMAX_DELAY))
        {
            printf("Logging Received: %d\n",
                   receivedData);
        }
    }
}

/* ---------------- PHASE CONTROL TASK ---------------- */

void PhaseControlTask(void *pvParameters)
{
    while (1)
    {
        /* Launch Phase */

        currentPhase = 1;

        printf("\n====================\n");
        printf("LAUNCH PHASE\n");
        printf("====================\n");

        vTaskPrioritySet(telemetryHandle, 3);
        vTaskPrioritySet(loggingHandle, 1);

        printf("Telemetry Priority = 3\n");
        printf("Logging Priority = 1\n");

        vTaskDelay(pdMS_TO_TICKS(10000));

        /* Ascent Phase */

        currentPhase = 2;

        printf("\n====================\n");
        printf("ASCENT PHASE\n");
        printf("====================\n");

        vTaskPrioritySet(telemetryHandle, 3);
        vTaskPrioritySet(loggingHandle, 1);

        printf("Telemetry Priority = 3\n");
        printf("Logging Priority = 1\n");

        vTaskDelay(pdMS_TO_TICKS(10000));

        /* Orbit Phase */

        currentPhase = 3;

        printf("\n====================\n");
        printf("ORBIT PHASE\n");
        printf("====================\n");

        vTaskPrioritySet(telemetryHandle, 2);
        vTaskPrioritySet(loggingHandle, 2);

        printf("Telemetry Priority = 2\n");
        printf("Logging Priority = 2\n");

        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

/* ---------------- MAIN ---------------- */

int main(void)
{
    /* Create Queue */

    telemetryQueue = xQueueCreate(5, sizeof(int));

    if (telemetryQueue == NULL)
    {
        printf("Queue Creation Failed!\n");
        return 1;
    }

    /* Create Tasks */

    xTaskCreate(
        TelemetryTask,
        "Telemetry",
        1024,
        NULL,
        2,
        &telemetryHandle);

    xTaskCreate(
        LoggingTask,
        "Logging",
        1024,
        NULL,
        1,
        &loggingHandle);

    xTaskCreate(
        PhaseControlTask,
        "PhaseControl",
        1024,
        NULL,
        4,
        NULL);

    /* Start Scheduler */

    vTaskStartScheduler();

    while (1);
}
