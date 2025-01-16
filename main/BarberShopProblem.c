
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "hd44780.h"
#include "esp_log.h"
#include "LL_Helper.h"


// Constants
#define CUSTOMERS 11
#define WAITING_ROOM_CAPACITY 4
#define CUSTOMER_INTERVAL_MIN 10000
#define CUSTOMER_INTERVAL_MAX 20000
#define HAIRCUT_TIME 10
#define BARBER_MIN_SLEEP 50000 
#define BARBERS 1

// Handles
static QueueHandle_t waitingRoom;

SemaphoreHandle_t ledMutex;

const int blueLEDs[2] = {LED_BLUE_B1, LED_BLUE_B2};
const int redLEDs[2] = {LED_RED_B1, LED_RED_B2};

// Global variables
bool barbersSleeping[BARBERS] = {1};


const char line1[] = "Barbers";
const char line2[] = "sleeping";
const char wait[] = "Waiting:";

// Function prototypes
void lcd_check(void);
void led_check(void);
void barber_task(void *arg);
void customer_task(void *arg);

void lcd_check_B(void)
{
    int currentlyWaiting = WAITING_ROOM_CAPACITY - uxQueueSpacesAvailable(waitingRoom);
    // Očisti LCD ekran
    hd44780_clear(&lcd);

    if (currentlyWaiting == 0)
    {
        // Prikaz: Barbers sleeping
        hd44780_gotoxy(&lcd, 0, 0);
        hd44780_puts(&lcd, line1);

        hd44780_gotoxy(&lcd, 0, 1);
        hd44780_puts(&lcd, line2);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    else
    {
        // Prikaz: Waiting:<broj>
        hd44780_gotoxy(&lcd, 0, 0);
        hd44780_puts(&lcd, wait);

        hd44780_gotoxy(&lcd, 0, 1);

        // Pretvori broj u string i ispiši ga
        char waitingCount[5];
        snprintf(waitingCount, sizeof(waitingCount), "%d", currentlyWaiting);
        hd44780_puts(&lcd, waitingCount);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void led_check_B()
{
    int spacesAvailable = uxQueueSpacesAvailable(waitingRoom);

    if (xSemaphoreTake(ledMutex, portMAX_DELAY) == pdTRUE)
    {
        gpio_set_level(LED_GREEN, spacesAvailable > 0 ? 1 : 0);

        for (int i = 0; i < BARBERS; i++)
        {
            gpio_set_level(blueLEDs[i], barbersSleeping[i] == 0 ? 1 : 0);
            gpio_set_level(redLEDs[i], barbersSleeping[i] == 0 ? 0 : 1);
        }

        lcd_check_B();

        // Dodaj kašnjenje za vizualizaciju
        vTaskDelay(pdMS_TO_TICKS(500)); // 50 ms odgode

        xSemaphoreGive(ledMutex);
    }
}

void barber_task(void *arg)
{
    int barberID = (int)arg;
    int customerID;

    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(100));
        if (xQueueReceive(waitingRoom, &customerID, 5000) == pdPASS)
        {
            barbersSleeping[barberID] = 0;
            ESP_LOGI("Barber", "Barber %d is serving customer %d.", barberID, customerID);

            vTaskDelay(pdMS_TO_TICKS(HAIRCUT_TIME));

            ESP_LOGI("Barber", "Barber %d finished serving customer %d.", barberID, customerID);

            led_check_B();
        }
        else
        {
            barbersSleeping[barberID] = 1;
            ESP_LOGI("Barber", "Barber %d is sleeping.", barberID);
            led_check_B();
            vTaskDelay(pdMS_TO_TICKS(BARBER_MIN_SLEEP));
        }
    }
}
void customer_task(void *arg)
{
    int customerID = (int)arg;

    while (1)
    {
        int arrivalDelay = CUSTOMER_INTERVAL_MIN + (rand() % (CUSTOMER_INTERVAL_MAX - CUSTOMER_INTERVAL_MIN));
        vTaskDelay(pdMS_TO_TICKS(arrivalDelay));

        ESP_LOGI("Customer", "Customer %d enters the shop.", customerID);

        if (xQueueSend(waitingRoom, &customerID, 0) == pdPASS)
        {
            ESP_LOGI("Customer", "Customer %d sits in the waiting room.", customerID);
            led_check_B();
        }
        else
        {
            ESP_LOGI("Customer", "Customer %d leaves the shop.", customerID);
        }
    }
}

void barber_run(void *arg)
{

   // vTaskDelay(pdMS_TO_TICKS(5000));

    ledMutex = xSemaphoreCreateMutex();
    if (ledMutex == NULL)
    {
        ESP_LOGE("LED", "Failed to create mutex for LEDs.");
        return;
    }

    waitingRoom = xQueueCreate(WAITING_ROOM_CAPACITY, sizeof(int));
    if (waitingRoom == NULL)
    {
        ESP_LOGE("Main", "Failed to create waiting room queue.");
        return;
    }

    for (int i = 0; i < BARBERS; i++)
    {
        char taskName[16];
        snprintf(taskName, sizeof(taskName), "Barber %d", i);
        xTaskCreate(barber_task, taskName, 2048, (void *)i, 4, NULL);
    }

    for (int i = 0; i < CUSTOMERS; i++)
    {
        char taskName[16];
        snprintf(taskName, sizeof(taskName), "Customer %d", i);
        xTaskCreate(customer_task, taskName, 2048, (void *)i, 3, NULL);
    }
}