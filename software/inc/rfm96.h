#ifndef LORA_RFM95_H_
#define LORA_RFM95_H_

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "hardware/spi.h"


#define TX_TIMEOUT_MS       5000   // tempo máximo esperando TxDone


typedef struct {
    spi_inst_t *spi_instance;
    uint pin_miso;
    uint pin_cs;
    uint pin_sck;
    uint pin_mosi;
    uint pin_rst;
    uint pin_dio0;
    long frequency; // Frequência em Hz (ex: 915E6)
} rfm96_config_t;

bool lora_init(rfm96_config_t config);
bool lora_send(const char *msg);
int lora_receive(char *buf, size_t maxlen);
void lora_start_rx_continuous(void);
bool lora_send_bytes(const uint8_t *data, size_t len);
int lora_receive_bytes(uint8_t *buf, size_t maxlen);
int lora_get_rssi(void);

#endif