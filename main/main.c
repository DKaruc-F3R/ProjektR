
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "LL_Helper.h"

volatile int menu_index = 0;
volatile bool is_running = false;

#define MENU_ITEMS 4

const char *menu_options[] = {
    "1. Barbe",
    "rs",
    "2. Philo",
    "sophers"};

TaskHandle_t active_task = NULL;

extern void barber_run(void *arg);
extern void philosophers_run(void *arg);

bool interrupt_active = false;

void IRAM_ATTR interrupt_handler(void *arg)
{
    printf("\nInterrupt triggered: Returning to main menu...\n");
    hd44780_clear(&lcd);
    hd44780_gotoxy(&lcd, 0, 0);
    hd44780_puts(&lcd, "INTERRUPT");
    hd44780_gotoxy(&lcd, 0, 1);
    hd44780_puts(&lcd, "T");
    is_running = false;

    if (active_task != NULL)
    {
        vTaskDelete(active_task);
        active_task = NULL;
    }

    menu_index = 0;
    interrupt_active = true;
}

void display_menu(void)
{

    hd44780_clear(&lcd);
    hd44780_gotoxy(&lcd, 0, 0);
    hd44780_puts(&lcd, menu_options[menu_index]);
    hd44780_gotoxy(&lcd, 0, 1);
    hd44780_puts(&lcd, menu_options[menu_index + 1]);
}

void nav_menu(void)
{

    vTaskDelay(pdMS_TO_TICKS(300)); // Debounce

    if (!is_running)
    {
        menu_index = (menu_index + 2) % MENU_ITEMS;
        display_menu();
        printf("Menu: %s%s\n", menu_options[menu_index], menu_options[menu_index + 1]);
    }
}

void conf_menu(void)
{
    vTaskDelay(pdMS_TO_TICKS(300)); // Debounce
    if (!is_running)
    {
        is_running = true;
        if (menu_index == 0)
        {
            printf("\nLaunching Sleeping Barber problem.\n");
            xTaskCreate(barber_run, "BarberShop", 4096, NULL, 5, &active_task);
        }
        else if (menu_index == 2)
        {
            printf("\nLaunching Dining Philosophers problem.\n");
            xTaskCreate(philosophers_run, "Philosophers", 4096, NULL, 5, &active_task);
        }

        gpio_isr_handler_add(BUTTON_CONFIRM, interrupt_handler, NULL);
    }
}

void button_init(void)
{
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_NEGEDGE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << BUTTON_NAVIGATE) | (1ULL << BUTTON_CONFIRM),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_ENABLE};

    gpio_config(&io_conf);

    gpio_install_isr_service(0);

    gpio_isr_handler_add(BUTTON_NAVIGATE, (void *)nav_menu, NULL);
    gpio_isr_handler_add(BUTTON_CONFIRM, (void *)conf_menu, NULL);
}

void app_main(void)
{
    lcd_init();
    led_init();
    button_init();

    printf("Main menu: Use Left button to navigate and Right button to confirm\n");

    hd44780_clear(&lcd);
    hd44780_gotoxy(&lcd, 0, 0);
    hd44780_puts(&lcd, "Select: ");

    nav_menu();

    while (1)
    {
        if (!is_running)
        {
            vTaskDelay(pdMS_TO_TICKS(100));
        }
        if(interrupt_active){
            interrupt_active = false;
            gpio_isr_handler_add(BUTTON_CONFIRM, (void *)conf_menu, NULL);
            vTaskDelay(pdMS_TO_TICKS(300));
        }
        vTaskDelay(pdMS_TO_TICKS(300));

    }
}
