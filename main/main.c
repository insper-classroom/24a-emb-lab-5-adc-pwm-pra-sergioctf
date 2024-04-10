/*
 * LED blink with FreeRTOS
 */
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>

#include "pico/stdlib.h"
#include <stdio.h>
#include "hardware/adc.h"

#include <math.h>
#include <stdlib.h>

QueueHandle_t xQueueAdc;

typedef struct adc {
    int axis;
    int val;
} adc_t;

void write_package(adc_t data) {
    int val = data.val;
    int msb = val >> 8;
    int lsb = val & 0xFF ;

    uart_putc_raw(uart0, data.axis); 
    uart_putc_raw(uart0, lsb);
    uart_putc_raw(uart0, msb); 
    uart_putc_raw(uart0, -1); 
}

void adc_task_x(void *p) {
    adc_init();
    adc_gpio_init(27); // X
    
    while (1) {
        //X
        adc_select_input(1); // Select ADC input 1 (GPIO27)
        int x = adc_read();
        //printf("X: %d V\n", x);

        struct adc x_pos = {0,x};
        xQueueSend(xQueueAdc, &x_pos, portMAX_DELAY);

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void adc_task_y(void *p) {
    adc_init();
    adc_gpio_init(28); //Y

    while (1) {
        //Y
        adc_select_input(2); // Select ADC input 2 (GPIO28)
        int y = adc_read();
        //printf("Y: %d V\n", y);

        struct adc y_pos = {1,y};
        xQueueSend(xQueueAdc, &y_pos, portMAX_DELAY);

        vTaskDelay(pdMS_TO_TICKS(100));

    }
}






void uart_task(void *p) {
    adc_t data;

    while (1) {
        xQueueReceive(xQueueAdc, &data, portMAX_DELAY);
        //printf("Axis: %d, Value: %d\n", data.axis, data.val);

        //Calcula a deadzone
        data.val = (data.val-2047)/8;
        int zone_limit = 80;
        if (data.val <=zone_limit && data.val >= -1*(zone_limit)) {
            data.val = 0;
        }

        write_package(data);
    }
}

int main() {
    stdio_init_all();

    xQueueAdc = xQueueCreate(32, sizeof(adc_t));

    xTaskCreate(adc_task_x, "ADC_Task x", 256, NULL, 1, NULL);

    xTaskCreate(adc_task_y, "ADX_Task y", 256, NULL, 1, NULL);

    xTaskCreate(uart_task, "uart_task", 4096, NULL, 1, NULL);

    vTaskStartScheduler();

    while (true)
        ;
}
