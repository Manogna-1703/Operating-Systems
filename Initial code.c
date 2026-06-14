#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* ---------------- GLOBAL VARIABLES ---------------- */

QueueHandle_t telemetryQueue;
QueueHandle_t emergencyQueue;

TaskHandle_t telemetryHandle;
TaskHandle_t loggingHandle;
TaskHandle_t emergencyHandle;

int currentPhase = 1;

/* ---------------- TELEMETRY TASK ---------------- */

void TelemetryTask(void *pvParameters)
{
    int data = 0;

    while (1)
    {
        data++;

        printf("[Telemetry] Sent: %d\n", data);

        xQueueSend(telemetryQueue, &data, 0);

        if (currentPhase == 1)
        {
            vTaskDelay(pdMS_TO_TICKS(500));   // Launch
        }
        else if (currentPhase == 2)
        {
            vTaskDelay(pdMS_TO_TICKS(1000));  // Ascent
        }
        else
        {
            vTaskDelay(pdMS_TO_TICKS(2000));  // Orbit
        }
    }
}

/* ---------------- EMERGENCY TASK ---------------- */

void EmergencyTask(void *pvParameters)
{
    int emergencyCode = 100;

    while (1)
    {
        emergencyCode++;

        printf("[Emergency] ALERT! Code %d\n",
               emergencyCode);

        xQueueSend(emergencyQueue,
                   &emergencyCode,
                   0);

        vTaskDelay(pdMS_TO_TICKS(7000));
    }
}

/* ---------------- LOGGING TASK ---------------- */

void LoggingTask(void *pvParameters)
{
    int telemetryData;
    int emergencyData;

    while (1)
    {
        if (xQueueReceive(telemetryQueue,
                          &telemetryData,
                          0))
        {
            printf("[Logging] Telemetry = %d\n",
                   telemetryData);
        }

        if (xQueueReceive(emergencyQueue,
                          &emergencyData,
                          0))
        {
            printf("[Logging] EMERGENCY = %d\n",
                   emergencyData);
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

/* ---------------- PHASE CONTROL TASK ---------------- */

void PhaseControlTask(void *pvParameters)
{
    while (1)
    {
        /* LAUNCH */

        currentPhase = 1;

        printf("\n====================\n");
        printf("LAUNCH PHASE\n");
        printf("====================\n");

        vTaskPrioritySet(emergencyHandle, 4);
        vTaskPrioritySet(telemetryHandle, 3);
        vTaskPrioritySet(loggingHandle, 1);

        printf("Emergency Priority = 4\n");
        printf("Telemetry Priority = 3\n");
        printf("Logging Priority = 1\n");

        vTaskDelay(pdMS_TO_TICKS(10000));

        /* ASCENT */

        currentPhase = 2;

        printf("\n====================\n");
        printf("ASCENT PHASE\n");
        printf("====================\n");

        vTaskPrioritySet(emergencyHandle, 4);
        vTaskPrioritySet(telemetryHandle, 3);
        vTaskPrioritySet(loggingHandle, 1);

        printf("Emergency Priority = 4\n");
        printf("Telemetry Priority = 3\n");
        printf("Logging Priority = 1\n");

        vTaskDelay(pdMS_TO_TICKS(10000));

        /* ORBIT */

        currentPhase = 3;

        printf("\n====================\n");
        printf("ORBIT PHASE\n");
        printf("====================\n");

        vTaskPrioritySet(emergencyHandle, 3);
        vTaskPrioritySet(telemetryHandle, 2);
        vTaskPrioritySet(loggingHandle, 2);

        printf("Emergency Priority = 3\n");
        printf("Telemetry Priority = 2\n");
        printf("Logging Priority = 2\n");

        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

/* ---------------- MAIN ---------------- */

int main(void)
{
    telemetryQueue = xQueueCreate(5, sizeof(int));
    emergencyQueue = xQueueCreate(5, sizeof(int));

    if ((telemetryQueue == NULL) ||
        (emergencyQueue == NULL))
    {
        printf("Queue Creation Failed!\n");
        return 1;
    }

    xTaskCreate(
        TelemetryTask,
        "Telemetry",
        1024,
        NULL,
        2,
        &telemetryHandle);

    xTaskCreate(
        EmergencyTask,
        "Emergency",
        1024,
        NULL,
        4,
        &emergencyHandle);

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
        5,
        NULL);

    vTaskStartScheduler();

    while (1);
}
