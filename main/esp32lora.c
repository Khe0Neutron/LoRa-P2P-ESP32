#include <stdio.h>
#include "ssd1306.h" //oled
#include "string.h"
#include "lora.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define MESSAGE_LENGTH 240

SSD1306_t screen;
TaskHandle_t xHandleRXTask;
uint8_t msg[MESSAGE_LENGTH];
int packets = 0;
int rssi = 0;

void screen_init() {
    i2c_master_init(&screen, CONFIG_SDA_GPIO, CONFIG_SCL_GPIO, CONFIG_RESET_GPIO);
    ssd1306_init(&screen, 128, 64);
    ssd1306_contrast(&screen, 0xFF); //CONTRASTE DE BRILLO
}

void screen_clear(){
    ssd1306_clear_screen(&screen, false); //LIMPIAR PANTALLA
}

void screen_print(char * str, int page) {
    ssd1306_clear_line(&screen, page, false);  //LIMPIAR EL PRIMER RENGLON SIN REVERSO FALSE
    ssd1306_display_text(&screen, page, str, strlen(str), false);
}

void task_rx(void *p) {
    int len;
    char packets_count[64];   //64 por la pantalla que tenemos que es de 128x64
    char rssi_str[64];
    for(;;) {
        lora_receive();
        while(lora_received()) {
            len = lora_receive_packet(msg, MESSAGE_LENGTH);
            msg[len] = 0;
            printf("Receive msg: %s, len: %d\n", msg, len);
            rssi = lora_packet_rssi(); //intensidad de la señal

            packets++; //cada vez que llega un nuevo paquete
            
            sprintf(packets_count, "Count: %d", packets);
            sprintf(rssi_str, "RSSI: %d dBm", rssi);
            screen_print(packets_count, 0);
            screen_print(rssi_str, 1);
        }
        vTaskDelay(1); //delay para que no se quede en un bucle el freeRTOS
    }
}

void send_msg(char * msg, int size) {
    printf("send packet: %s\n", msg);
    vTaskSuspend(xHandleRXTask);
    lora_send_packet((uint8_t *)msg, size);
    vTaskResume(xHandleRXTask);
    printf("packet sent!\n");
}

void task_tx(void *p) {
    for(;;) {
        send_msg("Ping!", 5);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}

void lora_config_init() {
    printf("Estas en lora config init!\n");
    lora_init();
    lora_set_frequency(915e6);
    lora_enable_crc();
    xTaskCreate(&task_rx, "task_rx", 2048, NULL, 5, &xHandleRXTask);
    xTaskCreate(&task_tx, "task_tx", 2048, NULL, 5, NULL);
}

void app_main(void)
{
    screen_init();
    screen_clear();
    screen_print("Hola mundo!", 0);
    lora_config_init();
}


