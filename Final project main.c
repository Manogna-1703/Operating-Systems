#define _CRT_SECURE_NO_WARNINGS

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

#define PHASE_MESSAGE_LIMIT 15
#define SENSOR_COUNT 12

QueueHandle_t telemetryQueue;

TaskHandle_t telemetryHandle;
TaskHandle_t loggingHandle;
TaskHandle_t phaseHandle;
TaskHandle_t emergencyHandle;
TaskHandle_t monitorHandle;

int currentPhase = LAUNCH;
int sentCount = 0;
int receivedCount = 0;
int faultCount = 0;
int emergencyCount = 0;
int emergencyFlag = 0;
int phaseMessageCount = 0;

volatile float latestSensorValue = 0.0f;
char latestSensorName[30] = "";
char latestSensorUnit[10] = "";

typedef struct
{
    uint32_t timestamp;
    char sensorName[30];
    float sensorValue;
    char unit[10];
    int phase;
} MissionMessage;

typedef struct
{
    char phase[15];
    char sensorName[30];
    char unit[10];
    char normalRange[40];
    char threshold[50];
    float currentValue;
    char status[15];
} SensorTable;

SensorTable sensorTable[SENSOR_COUNT] =
{
    {"Launch", "EngineTemp", "C", "700-899 C", "> 850 C", 0, "NORMAL"},
    {"Launch", "FuelPressure", "PSI", "3000-3999 PSI", "> 3900 PSI", 0, "NORMAL"},
    {"Launch", "Thrust", "kN", "500-899 kN", "< 550 kN", 0, "NORMAL"},
    {"Launch", "Vibration", "g", "2-9 g", "> 8 g", 0, "NORMAL"},

    {"Ascent", "Altitude", "km", "50-349 km", "< 80 km", 0, "NORMAL"},
    {"Ascent", "Velocity", "m/s", "1000-5999 m/s", "< 1500 m/s", 0, "NORMAL"},
    {"Ascent", "FuelRemaining", "%", "20-99 %", "< 25 %", 0, "NORMAL"},
    {"Ascent", "ExternalTemp", "C", "-100 to 99 C", "< -80 C or > 80 C", 0, "NORMAL"},

    {"Orbit", "Battery", "%", "40-99 %", "< 45 %", 0, "NORMAL"},
    {"Orbit", "SolarOutput", "W", "1000-1499 W", "< 1100 W", 0, "NORMAL"},
    {"Orbit", "CabinTemp", "C", "18-27 C", "< 15 C or > 30 C", 0, "NORMAL"},
    {"Orbit", "Radiation", "mSv", "0-14 mSv", "> 12 mSv", 0, "NORMAL"}
};

void formatTime(uint32_t ticks, char* timeStr)
{
    uint32_t totalSeconds = ticks / 1000;
    uint32_t hours = totalSeconds / 3600;
    uint32_t minutes = (totalSeconds % 3600) / 60;
    uint32_t seconds = totalSeconds % 60;

    sprintf(timeStr, "%02lu:%02lu:%02lu",
        (unsigned long)hours,
        (unsigned long)minutes,
        (unsigned long)seconds);
}

const char* getPhaseName(int phase)
{
    if (phase == LAUNCH) return "LAUNCH";
    if (phase == ASCENT) return "ASCENT";
    return "ORBIT";
}

int getTelemetryDelay(void)
{
    if (currentPhase == LAUNCH) return 500;
    if (currentPhase == ASCENT) return 1000;
    return 2000;
}

int getTelemetryPriority(void)
{
    if (currentPhase == LAUNCH) return 3;
    if (currentPhase == ASCENT) return 2;
    return 1;
}

int CheckEmergencyCondition(const char* sensorName, float value)
{
    if (strcmp(sensorName, "EngineTemp") == 0 && value > 850)
        return 1;

    if (strcmp(sensorName, "FuelPressure") == 0 && value > 3900)
        return 1;

    if (strcmp(sensorName, "Thrust") == 0 && value < 550)
        return 1;

    if (strcmp(sensorName, "Vibration") == 0 && value > 8)
        return 1;

    if (strcmp(sensorName, "Altitude") == 0 && value < 80)
        return 1;

    if (strcmp(sensorName, "Velocity") == 0 && value < 1500)
        return 1;

    if (strcmp(sensorName, "FuelRemaining") == 0 && value < 25)
        return 1;

    if (strcmp(sensorName, "ExternalTemp") == 0 && (value < -80 || value > 80))
        return 1;

    if (strcmp(sensorName, "Battery") == 0 && value < 45)
        return 1;

    if (strcmp(sensorName, "SolarOutput") == 0 && value < 1100)
        return 1;

    if (strcmp(sensorName, "CabinTemp") == 0 && (value < 15 || value > 30))
        return 1;

    if (strcmp(sensorName, "Radiation") == 0 && value > 12)
        return 1;

    return 0;
}

void UpdateSensorTable(const char* sensorName, float value)
{
    int i;

    for (i = 0; i < SENSOR_COUNT; i++)
    {
        if (strcmp(sensorTable[i].sensorName, sensorName) == 0)
        {
            sensorTable[i].currentValue = value;

            if (CheckEmergencyCondition(sensorName, value))
            {
                strcpy(sensorTable[i].status, "EMERGENCY");
            }
            else
            {
                strcpy(sensorTable[i].status, "NORMAL");
            }

            break;
        }
    }
}

void PrintTelemetryThresholdTable(void)
{
    int i;

    printf("\n============================================================================================\n");
    printf(" LIVE TELEMETRY THRESHOLD TABLE\n");
    printf("============================================================================================\n");
    printf("%-10s %-18s %-15s %-22s %-25s %-12s\n",
        "Phase", "Sensor", "Value", "Normal Range", "Emergency Threshold", "Status");
    printf("--------------------------------------------------------------------------------------------\n");

    for (i = 0; i < SENSOR_COUNT; i++)
    {
        printf("%-10s %-18s %7.2f %-6s %-22s %-25s %-12s\n",
            sensorTable[i].phase,
            sensorTable[i].sensorName,
            sensorTable[i].currentValue,
            sensorTable[i].unit,
            sensorTable[i].normalRange,
            sensorTable[i].threshold,
            sensorTable[i].status);
    }

    printf("============================================================================================\n\n");
}

void GenerateTelemetry(MissionMessage* msg)
{
    msg->timestamp = xTaskGetTickCount();
    msg->phase = currentPhase;

    if (currentPhase == LAUNCH)
    {
        switch (rand() % 4)
        {
        case 0:
            strcpy(msg->sensorName, "EngineTemp");
            msg->sensorValue = 700 + rand() % 200;
            strcpy(msg->unit, "C");
            break;

        case 1:
            strcpy(msg->sensorName, "FuelPressure");
            msg->sensorValue = 3000 + rand() % 1000;
            strcpy(msg->unit, "PSI");
            break;

        case 2:
            strcpy(msg->sensorName, "Vibration");
            msg->sensorValue = 2 + rand() % 8;
            strcpy(msg->unit, "g");
            break;

        case 3:
            strcpy(msg->sensorName, "Thrust");
            msg->sensorValue = 500 + rand() % 400;
            strcpy(msg->unit, "kN");
            break;
        }
    }
    else if (currentPhase == ASCENT)
    {
        switch (rand() % 4)
        {
        case 0:
            strcpy(msg->sensorName, "Altitude");
            msg->sensorValue = 50 + rand() % 300;
            strcpy(msg->unit, "km");
            break;

        case 1:
            strcpy(msg->sensorName, "Velocity");
            msg->sensorValue = 1000 + rand() % 5000;
            strcpy(msg->unit, "m/s");
            break;

        case 2:
            strcpy(msg->sensorName, "FuelRemaining");
            msg->sensorValue = 20 + rand() % 80;
            strcpy(msg->unit, "%");
            break;

        case 3:
            strcpy(msg->sensorName, "ExternalTemp");
            msg->sensorValue = -100 + rand() % 200;
            strcpy(msg->unit, "C");
            break;
        }
    }
    else
    {
        switch (rand() % 4)
        {
        case 0:
            strcpy(msg->sensorName, "Battery");
            msg->sensorValue = 40 + rand() % 60;
            strcpy(msg->unit, "%");
            break;

        case 1:
            strcpy(msg->sensorName, "SolarOutput");
            msg->sensorValue = 1000 + rand() % 500;
            strcpy(msg->unit, "W");
            break;

        case 2:
            strcpy(msg->sensorName, "CabinTemp");
            msg->sensorValue = 18 + rand() % 10;
            strcpy(msg->unit, "C");
            break;

        case 3:
            strcpy(msg->sensorName, "Radiation");
            msg->sensorValue = rand() % 15;
            strcpy(msg->unit, "mSv");
            break;
        }
    }
}

void TelemetryTask(void* pvParameters)
{
    MissionMessage msg;
    char timeStr[20];

    while (1)
    {
        GenerateTelemetry(&msg);
        formatTime(msg.timestamp, timeStr);

        latestSensorValue = msg.sensorValue;
        strcpy(latestSensorName, msg.sensorName);
        strcpy(latestSensorUnit, msg.unit);

        UpdateSensorTable(msg.sensorName, msg.sensorValue);

        if (CheckEmergencyCondition(msg.sensorName, msg.sensorValue))
        {
            emergencyFlag = 1;
            faultCount++;

            printf("\n[TIME:%s] [FAULT DETECTED]\n", timeStr);
            printf("Sensor: %s\n", msg.sensorName);
            printf("Value : %.2f %s\n", msg.sensorValue, msg.unit);
            printf("Emergency task will be activated.\n\n");
            fflush(stdout);
        }

        if (xQueueSend(telemetryQueue, &msg, 0) == pdPASS)
        {
            sentCount++;
            phaseMessageCount++;

            printf("[TIME:%s] [PHASE %d: %s] Telemetry Sent | %s = %.2f %s | Sent Count: %d | Phase Count: %d\n",
                timeStr,
                msg.phase,
                getPhaseName(msg.phase),
                msg.sensorName,
                msg.sensorValue,
                msg.unit,
                sentCount,
                phaseMessageCount);

            fflush(stdout);
        }
        else
        {
            formatTime(xTaskGetTickCount(), timeStr);
            printf("[TIME:%s] [QUEUE WARNING] Queue is full. Message lost.\n", timeStr);
            fflush(stdout);
        }

        vTaskDelay(pdMS_TO_TICKS(getTelemetryDelay()));
    }
}

void LoggingTask(void* pvParameters)
{
    MissionMessage receivedMsg;
    char timeStr[20];

    while (1)
    {
        if (xQueueReceive(telemetryQueue, &receivedMsg, portMAX_DELAY) == pdPASS)
        {
            receivedCount++;
            formatTime(receivedMsg.timestamp, timeStr);

            printf("[TIME:%s] [LOGGING] Message Received | Phase: %s | %s = %.2f %s | Received Count: %d\n",
                timeStr,
                getPhaseName(receivedMsg.phase),
                receivedMsg.sensorName,
                receivedMsg.sensorValue,
                receivedMsg.unit,
                receivedCount);

            fflush(stdout);
        }
    }
}

void PhaseControlTask(void* pvParameters)
{
    char timeStr[20];

    while (1)
    {
        if (phaseMessageCount >= PHASE_MESSAGE_LIMIT)
        {
            phaseMessageCount = 0;
            formatTime(xTaskGetTickCount(), timeStr);

            if (currentPhase == LAUNCH)
            {
                currentPhase = ASCENT;
                vTaskPrioritySet(telemetryHandle, 2);

                printf("\n[TIME:%s] [SYSTEM TRIGGER] Mission changed: LAUNCH -> ASCENT\n", timeStr);
                printf("[UPDATE] Telemetry Priority = 2 | Delay = 1000 ms | Phase changed after 15 messages\n\n");
            }
            else if (currentPhase == ASCENT)
            {
                currentPhase = ORBIT;
                vTaskPrioritySet(telemetryHandle, 1);

                printf("\n[TIME:%s] [SYSTEM TRIGGER] Mission changed: ASCENT -> ORBIT\n", timeStr);
                printf("[UPDATE] Telemetry Priority = 1 | Delay = 2000 ms | Phase changed after 15 messages\n\n");
            }
            else
            {
                currentPhase = LAUNCH;
                vTaskPrioritySet(telemetryHandle, 3);

                printf("\n[TIME:%s] [SYSTEM TRIGGER] Mission restarted: ORBIT -> LAUNCH\n", timeStr);
                printf("[UPDATE] Telemetry Priority = 3 | Delay = 500 ms | Phase changed after 15 messages\n\n");
            }

            fflush(stdout);
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void EmergencyTask(void* pvParameters)
{
    char timeStr[20];

    while (1)
    {
        if (emergencyFlag == 1)
        {
            emergencyCount++;
            formatTime(xTaskGetTickCount(), timeStr);

            printf("\n=====================================\n");
            printf(" EMERGENCY TASK ACTIVATED\n");
            printf("=====================================\n");
            printf("Time               : %s\n", timeStr);
            printf("Emergency Priority : 4\n");
            printf("Sensor             : %s\n", latestSensorName);
            printf("Value              : %.2f %s\n", latestSensorValue, latestSensorUnit);
            printf("Response           : Emergency handling initiated\n");
            printf("Emergency Count    : %d\n", emergencyCount);
            printf("=====================================\n\n");

            fflush(stdout);

            emergencyFlag = 0;
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void SystemMonitorTask(void* pvParameters)
{
    char timeStr[20];

    while (1)
    {
        float successRate = 0.0f;

        if (sentCount > 0)
        {
            successRate = ((float)receivedCount / (float)sentCount) * 100.0f;
        }

        formatTime(xTaskGetTickCount(), timeStr);

        printf("\n====================================\n");
        printf(" SYSTEM MONITOR DASHBOARD\n");
        printf("====================================\n");
        printf("Time                : %s\n", timeStr);
        printf("Current Phase       : %s\n", getPhaseName(currentPhase));
        printf("Telemetry Priority  : %d\n", getTelemetryPriority());
        printf("Telemetry Delay     : %d ms\n", getTelemetryDelay());
        printf("Phase Message Count : %d\n", phaseMessageCount);
        printf("Messages Sent       : %d\n", sentCount);
        printf("Messages Received   : %d\n", receivedCount);
        printf("Message Loss        : %d\n", sentCount - receivedCount);
        printf("Success Rate        : %.2f%%\n", successRate);
        printf("Fault Count         : %d\n", faultCount);
        printf("Emergency Count     : %d\n", emergencyCount);
        printf("Queue Status        : %s\n", sentCount == receivedCount ? "NORMAL" : "WARNING");
        printf("System Health       : RUNNING\n");
        printf("Free Heap           : %u bytes\n", (unsigned int)xPortGetFreeHeapSize());
        printf("====================================\n");

        PrintTelemetryThresholdTable();

        fflush(stdout);

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

int main(void)
{
    printf("==============================================\n");
    printf(" Spacecraft Orbital Mission Communication System\n");
    printf(" FreeRTOS Final Project Simulation Started\n");
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
    xTaskCreate(EmergencyTask, "Emergency", 1000, NULL, 4, &emergencyHandle);
    xTaskCreate(SystemMonitorTask, "Monitor", 1000, NULL, 2, &monitorHandle);

    vTaskStartScheduler();

    while (1);

    return 0;
}
