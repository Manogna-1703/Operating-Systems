#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#define LAUNCH 1
#define ASCENT 2
#define ORBIT  3
#define EMERGENCY_THRESHOLD 170.0f
QueueHandle_t telemetryQueue;

TaskHandle_t telemetryHandle;
TaskHandle_t loggingHandle;
TaskHandle_t phaseHandle;
TaskHandle_t emergencyHandle;
TaskHandle_t monitorHandle;

int currentPhase = LAUNCH;
int sentCount = 0;
int receivedCount = 0;
volatile float latestSensorValue = 0.0f;
char latestSensorName[30] = "";

typedef struct
{
    uint32_t timestamp;
    char sensorName[30];
    float sensorValue;
    char unit[10];
    int phase;
} MissionMessage;

const char* getPhaseName(int phase)
{
    if (phase == LAUNCH) return "LAUNCH";
    if (phase == ASCENT) return "ASCENT";
    return "ORBIT";
}
void GenerateTelemetry(MissionMessage *msg)
{
    msg->phase = currentPhase;

    switch(currentPhase)
    {
        case LAUNCH:

            switch(rand()%4)
            {
                case 0:
                    strcpy(msg->sensorName, "EngineTemp");
                    msg->sensorValue = 700 + rand()%200;
                    strcpy(msg->unit, "C");
                    break;

                case 1:
                    strcpy(msg->sensorName, "FuelPressure");
                    msg->sensorValue = 3000 + rand()%1000;
                    strcpy(msg->unit, "PSI");
                    break;

                case 2:
                    strcpy(msg->sensorName, "Vibration");
                    msg->sensorValue = 2 + rand()%8;
                    strcpy(msg->unit, "g");
                    break;

                case 3:
                    strcpy(msg->sensorName, "Thrust");
                    msg->sensorValue = 500 + rand()%400;
                    strcpy(msg->unit, "kN");
                    break;
            }
            break;

        case ASCENT:

            switch(rand()%4)
            {
                case 0:
                    strcpy(msg->sensorName, "Altitude");
                    msg->sensorValue = 50 + rand()%300;
                    strcpy(msg->unit, "km");
                    break;

                case 1:
                    strcpy(msg->sensorName, "Velocity");
                    msg->sensorValue = 1000 + rand()%5000;
                    strcpy(msg->unit, "m/s");
                    break;

                case 2:
                    strcpy(msg->sensorName, "FuelRemaining");
                    msg->sensorValue = 20 + rand()%80;
                    strcpy(msg->unit, "%");
                    break;

                case 3:
                    strcpy(msg->sensorName, "ExternalTemp");
                    msg->sensorValue = -100 + rand()%200;
                    strcpy(msg->unit, "C");
                    break;
            }
            break;

        case ORBIT:

            switch(rand()%4)
            {
                case 0:
                    strcpy(msg->sensorName, "Battery");
                    msg->sensorValue = 40 + rand()%60;
                    strcpy(msg->unit, "%");
                    break;

                case 1:
                    strcpy(msg->sensorName, "SolarOutput");
                    msg->sensorValue = 1000 + rand()%500;
                    strcpy(msg->unit, "W");
                    break;

                case 2:
                    strcpy(msg->sensorName, "CabinTemp");
                    msg->sensorValue = 18 + rand()%10;
                    strcpy(msg->unit, "C");
                    break;

                case 3:
                    strcpy(msg->sensorName, "Radiation");
                    msg->sensorValue = rand()%15;
                    strcpy(msg->unit, "mSv");
                    break;
            }
            break;
    }
}

void TelemetryTask(void* pvParameters)
{
    MissionMessage msg;

    while (1)
    {
        msg.timestamp = xTaskGetTickCount();
        GenerateTelemetry(&msg);
        latestSensorValue = msg.sensorValue;
        strcpy(latestSensorName, msg.sensorName);
        if (xQueueSend(telemetryQueue, &msg, 0) == pdPASS)
        {
            sentCount++;
            printf("[TELEMETRY] %s | %s = %.2f %s | Sent Count: %d\n",
            getPhaseName(msg.phase),
            msg.sensorName,
            msg.sensorValue,
            msg.unit,
            sentCount);
            fflush(stdout);
        }

        if (currentPhase == LAUNCH)
        {
            vTaskDelay(pdMS_TO_TICKS(500));
        }
        else if (currentPhase == ASCENT)
        {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
        else
        {
            vTaskDelay(pdMS_TO_TICKS(2000));
        }
    }
}

void LoggingTask(void* pvParameters)
{
    MissionMessage receivedMsg;

    while (1)
    {
        if (xQueueReceive(telemetryQueue, &receivedMsg, portMAX_DELAY) == pdPASS)
        {
            receivedCount++;

            printf("[LOGGING] %s | %s = %.2f %s | Received Count: %d\n",
            getPhaseName(receivedMsg.phase), receivedMsg.sensorName, receivedMsg.sensorValue, receivedMsg.unit, receivedCount);
           fflush(stdout);
        }
    }
}

void PhaseControlTask(void* pvParameters)
{
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(5000));

        if (currentPhase == LAUNCH)
        {
            currentPhase = ASCENT;
            vTaskPrioritySet(telemetryHandle, 2);

            printf("\n[SYSTEM TRIGGER] Mission changed: LAUNCH -> ASCENT\n");
            printf("[UPDATE] Telemetry Priority = 2 | Delay = 1000 ms\n\n");
            fflush(stdout);
        }
        else if (currentPhase == ASCENT)
        {
            currentPhase = ORBIT;
            vTaskPrioritySet(telemetryHandle, 1);

            printf("\n[SYSTEM TRIGGER] Mission changed: ASCENT -> ORBIT\n");
            printf("[UPDATE] Telemetry Priority = 1 | Delay = 2000 ms\n\n");
            fflush(stdout);
        }
        else
        {
            currentPhase = LAUNCH;
            vTaskPrioritySet(telemetryHandle, 3);

            printf("\n[SYSTEM TRIGGER] Mission restarted: ORBIT -> LAUNCH\n");
            printf("[UPDATE] Telemetry Priority = 3 | Delay = 500 ms\n\n");
            fflush(stdout);
        }
    }
}

void EmergencyTask(void* pvParameters)
{
    while(1)
    {
        int emergency = 0;
        if(strcmp(latestSensorName,"EngineTemp")==0 &&
           latestSensorValue > 850)
            emergency = 1;
        if(strcmp(latestSensorName,"FuelPressure")==0 &&
           latestSensorValue > 3900)
            emergency = 1;
        if(strcmp(latestSensorName,"Radiation")==0 &&
           latestSensorValue > 12)
            emergency = 1;
        if(strcmp(latestSensorName,"Battery")==0 &&
           latestSensorValue < 45)
            emergency = 1;
        if(emergency)
        {
            printf("\n");
            printf("=====================================\n");
            printf("[EMERGENCY DETECTED]\n");
            printf("Sensor : %s\n", latestSensorName);
            printf("Value  : %.2f\n", latestSensorValue);
            printf("Emergency Response Activated\n");
            printf("=====================================\n\n");

            fflush(stdout);
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void SystemMonitorTask(void* pvParameters)
{
    while(1)
    {
        printf("\n===== SYSTEM STATUS =====\n");

        printf("Current Phase : %s\n",
               getPhaseName(currentPhase));

        printf("Messages Sent : %d\n",
               sentCount);

        printf("Messages Received : %d\n",
               receivedCount);

        printf("Free Heap : %u bytes\n",
               (unsigned int)xPortGetFreeHeapSize());

        printf("Telemetry Stack : %u\n",
               (unsigned int)uxTaskGetStackHighWaterMark(telemetryHandle));

        printf("Logging Stack : %u\n",
               (unsigned int)uxTaskGetStackHighWaterMark(loggingHandle));

        printf("Emergency Stack : %u\n",
               (unsigned int)uxTaskGetStackHighWaterMark(emergencyHandle));

        printf("=========================\n\n");

        fflush(stdout);

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

int main(void)
{
    printf("==============================================\n");
    printf(" Spacecraft Orbital Mission Communication System\n");
    printf(" FreeRTOS Simulation Started\n");
    printf("==============================================\n\n");
    fflush(stdout);

    telemetryQueue = xQueueCreate(10, sizeof(MissionMessage));

    if (telemetryQueue == NULL)
    {
        printf("Queue creation failed!\n");
        fflush(stdout);
        return 1;
    }

    xTaskCreate(TelemetryTask, "Telemetry", 1000, NULL, 3, &telemetryHandle);
    xTaskCreate(LoggingTask, "Logging", 1000, NULL, 2, &loggingHandle);
    xTaskCreate(PhaseControlTask, "PhaseControl", 1000, NULL, 1, &phaseHandle);
    xTaskCreate(EmergencyTask, "Emergency", 1000, NULL, 5, &emergencyHandle);
    xTaskCreate(SystemMonitorTask, "Monitor", 1000, NULL,  2, &monitorHandle);

    vTaskStartScheduler();

    while (1);

    return 0;
}
