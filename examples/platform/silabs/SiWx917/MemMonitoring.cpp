/*
 *
 *    Copyright (c) 2021 Project CHIP Authors
 *    All rights reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include "MemMonitoring.h"

#include "AppConfig.h"
#include "FreeRTOS.h"
#include <platform/CHIPDeviceLayer.h>


static StackType_t monitoringStack[MONITORING_STACK_SIZE_byte / sizeof(StackType_t)];
static StaticTask_t monitoringTaskStruct;

size_t nbAllocSuccess        = 0;
size_t nbFreeSuccess         = 0;
size_t largestBlockAllocated = 0;

void MemMonitoring::startHeapMonitoring()
{
    xTaskCreateStatic(HeapMonitoring, "Monitoring", MONITORING_STACK_SIZE_byte / sizeof(StackType_t), NULL, 1, monitoringStack,
                      &monitoringTaskStruct);
}

void MemMonitoring::HeapMonitoring(void * pvParameter)
{

    UBaseType_t appTaskValue;
    UBaseType_t linkLayerTaskValue;
    UBaseType_t openThreadTaskValue;
    UBaseType_t eventLoopTaskValue;

    TaskHandle_t eventLoopHandleStruct = xTaskGetHandle(CHIP_DEVICE_CONFIG_CHIP_TASK_NAME);
    TaskHandle_t otTaskHandle          = xTaskGetHandle(CHIP_DEVICE_CONFIG_THREAD_TASK_NAME);
    TaskHandle_t appTaskHandle         = xTaskGetHandle(APP_TASK_NAME);

#if CHIP_SYSTEM_CONFIG_USE_LWIP
    UBaseType_t lwipTaskValue;
    TaskHandle_t lwipHandle = xTaskGetHandle(TCPIP_THREAD_NAME);
#endif // CHIP_SYSTEM_CONFIG_USE_LWIP

    while (1)
    {
        appTaskValue        = uxTaskGetStackHighWaterMark(appTaskHandle);
        openThreadTaskValue = uxTaskGetStackHighWaterMark(otTaskHandle);
        eventLoopTaskValue  = uxTaskGetStackHighWaterMark(eventLoopHandleStruct);
#if CHIP_SYSTEM_CONFIG_USE_LWIP
        lwipTaskValue = uxTaskGetStackHighWaterMark(lwipHandle);
#endif // CHIP_SYSTEM_CONFIG_USE_LWIP

        SILABS_LOG("=============================");
        SILABS_LOG("     ");
        SILABS_LOG("Largest Block allocated              0x%x", largestBlockAllocated);
        SILABS_LOG("Number Of Successful Alloc           0x%x", nbAllocSuccess);
        SILABS_LOG("Number Of Successful Frees           0x%x", nbFreeSuccess);
        SILABS_LOG("     ");
        SILABS_LOG("App Task most bytes ever Free         0x%x", (appTaskValue * 4));
        SILABS_LOG("Link Layer Task most bytes ever Free  0x%x", (linkLayerTaskValue * 4));
        SILABS_LOG("OpenThread Task most bytes ever Free  0x%x", (openThreadTaskValue * 4));
        SILABS_LOG("Event Loop Task most bytes ever Free  0x%x", (eventLoopTaskValue * 4));
#if CHIP_SYSTEM_CONFIG_USE_LWIP
        SILABS_LOG("LWIP Task most bytes ever Free        0x%x", (lwipTaskValue * 4));
#endif // CHIP_SYSTEM_CONFIG_USE_LWIP
        SILABS_LOG("     ");
        SILABS_LOG("=============================");
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

extern "C" void memMonitoringTrackAlloc(void * ptr, size_t size)
{
    if (ptr != NULL)
    {
        nbAllocSuccess++;
        if (largestBlockAllocated < size)
        {
            largestBlockAllocated = size;
        }
    }
}

extern "C" void memMonitoringTrackFree(void * ptr, size_t size)
{
    nbFreeSuccess++;
}
