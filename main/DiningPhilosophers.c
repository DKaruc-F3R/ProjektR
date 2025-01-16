#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "LL_Helper.h"
#include "hd44780.h"
#include "esp_log.h"

#define PHILOSOPHERS 5
#define EAT_TIME 8000
#define THINK_TIME 6000

static SemaphoreHandle_t monitorMutex;
static SemaphoreHandle_t cond[PHILOSOPHERS];


static int forks[PHILOSOPHERS] = {1, 1, 1, 1, 1};
static int timesEaten[PHILOSOPHERS] = {0, 0, 0, 0, 0};
static int thinking[PHILOSOPHERS] = {0, 0, 0, 0, 0};
static int eating[PHILOSOPHERS] = {0, 0, 0, 0, 0};

const int LEDS[PHILOSOPHERS] = {LED_BLUE_B1, LED_GREEN, LED_BLUE_B2, LED_RED_B2, LED_RED_B1};

void lcd_check_P(void)
{
    hd44780_clear(&lcd);
    hd44780_gotoxy(&lcd, 0, 0);
    char phil_T[8] = "";
    char phil_E[8] = "";
    for (int i = 0; i < PHILOSOPHERS; i++)
    {
        if (thinking[i] == 1)
        {
            char temp[4];
            snprintf(temp, sizeof(temp), "%d ", i);
            strncat(phil_T, temp, sizeof(phil_T) - strlen(phil_T) - 1);
        }

        if (eating[i] == 1)
        {
            char temp[4];
            snprintf(temp, sizeof(temp), "%d ", i);
            strncat(phil_E, temp, sizeof(phil_T) - strlen(phil_E) - 1);
        }
    }

    hd44780_puts(&lcd, "Thinking");
    vTaskDelay(pdMS_TO_TICKS(500));
    hd44780_puts(&lcd, phil_T);
    vTaskDelay(pdMS_TO_TICKS(500));
    hd44780_clear(&lcd);
    hd44780_puts(&lcd, "Eating");
    vTaskDelay(pdMS_TO_TICKS(500));
    hd44780_puts(&lcd, phil_E);
    vTaskDelay(pdMS_TO_TICKS(500));
}

void led_check_P(void *arg)
{
    while (1)
    {

        for (int i = 0; i < PHILOSOPHERS; i++)
        {
            gpio_set_level(LEDS[i], eating[i] == 1 ? 1 : 0);
        }

        lcd_check_P();

    }
}

void take_fork(int i)
{
    int L = i;
    int D = (L + 1) % PHILOSOPHERS;
    xSemaphoreTake(monitorMutex, portMAX_DELAY);
    while (forks[L] + forks[D] < 2)
    {
        xSemaphoreGive(monitorMutex);
        xSemaphoreTake(cond[i], portMAX_DELAY);
        xSemaphoreTake(monitorMutex, portMAX_DELAY);
    }

    forks[L] = 0;
    forks[D] = 0;
    xSemaphoreGive(monitorMutex);
}

void put_fork(int i)
{
    int L = i;
    int D = (L + 1) % PHILOSOPHERS;
    int LN = (L - 1 + PHILOSOPHERS) % PHILOSOPHERS;
    int DN = D;

    if (timesEaten[LN] <= timesEaten[DN])
    {
        xSemaphoreGive(cond[LN]);
        xSemaphoreGive(cond[DN]);
    }
    else
    {
        xSemaphoreGive(cond[DN]);
        xSemaphoreGive(cond[LN]);
    }

    xSemaphoreGive(monitorMutex);
}

void philosopher_task(void *arg)
{
    int philosopherID = (int)arg;
    while (1)
    {
        ESP_LOGI("Philosopher", "Philosopher %d is thinking.", philosopherID);
        thinking[philosopherID] = 1;
        eating[philosopherID] = 0;

        vTaskDelay(pdMS_TO_TICKS(THINK_TIME));

        ESP_LOGI("Philosopher", "Philosopher %d is attempting to take the forks.", philosopherID);

        take_fork(philosopherID);
        timesEaten[philosopherID]++;
        eating[philosopherID] = 1;
        thinking[philosopherID] = 0;
        ESP_LOGI("Philosopher", "Philosopher %d is eating. Times eaten: %d", philosopherID, timesEaten[philosopherID]);
        vTaskDelay(pdMS_TO_TICKS(EAT_TIME));

        ESP_LOGI("Philosopher", "Philosopher %d is putting down the forks.", philosopherID);
        put_fork(philosopherID);
    }
}

void philosophers_run(void *arg)
{
    monitorMutex = xSemaphoreCreateMutex();
    if (monitorMutex == NULL)
    {
        ESP_LOGE("LED", "Failed to create mutex for LEDs.");
        return;
    }

    for (int i = 0; i < PHILOSOPHERS; i++)
    {
        cond[i] = xSemaphoreCreateBinary();
        xSemaphoreTake(cond[i], 0);
    }

    for (int i = 0; i < PHILOSOPHERS; i++)
    {
        char taskName[16];
        snprintf(taskName, sizeof(taskName), "Philosopher %d", i);
        xTaskCreate(philosopher_task, taskName, 2048, (void *)i, 3, NULL);
    }

    xTaskCreate(led_check_P, "LED Check", 2048, NULL, 3, NULL);

}