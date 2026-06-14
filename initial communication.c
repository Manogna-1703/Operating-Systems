#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* Queue Handle */
QueueHandle_t telemetryQueue;

/* ---------------- TELEMETRY TASK ---------------- */

void TelemetryTask(void *pvParameters)
{
    int data = 0;

    while (1)
    {
        data++;

        printf("Telemetry Sent: %d\n", data);

        xQueueSend(telemetryQueue, &data, 0);

        vTaskDelay(pdMS_TO_TICKS(1000));
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

/* ---------------- MAIN ---------------- */

int main(void)
{
    /* Create Queue */
    telemetryQueue = xQueueCreate(5, sizeof(int));

    if (telemetryQueue == NULL)
    {
        printf("Queue creation failed!\n");
        return 1;
    }

    /* Create Tasks */

    xTaskCreate(
        TelemetryTask,
        "Telemetry",
        1024,
        NULL,
        2,
        NULL);

    xTaskCreate(
        LoggingTask,
        "Logging",
        1024,
        NULL,
        1,
        NULL);

    /* Start Scheduler */
    vTaskStartScheduler();

    /* Should never reach here */
    for (;;);
}
