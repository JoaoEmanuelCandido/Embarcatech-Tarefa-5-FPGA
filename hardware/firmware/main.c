// main.c - FPGA SoC (VexRiscv) bare-metal
#include <stdint.h>
#include <stdio.h>
#include "csr.h"    // gerado pelo LiteX (ex: csr.h, csr.c)
#include "i2c.h"    // drivers simples (se implementado)
#include "spi.h"
#include "timer.h"

#define NODE_ID 0x01
static uint16_t seq = 0;

int16_t aht10_read_temperature_raw(); // retorna temp*100
uint16_t aht10_read_humidity_raw();   // retorna hum*100

void rfm96_init();
void rfm96_send(uint8_t *data, int len);

uint8_t checksum(uint8_t *buf, int len) {
    uint8_t x = 0;
    for (int i=0;i<len;i++) x ^= buf[i];
    return x;
}

int main() {
    // Inicializações
    spi_init();   // inicializa SPI (LiteX CSR SPI)
    i2c_init();   // inicializa I2C (LiteX CSR I2C)
    timer_init();

    rfm96_init();

    while (1) {
        // Ler AHT10
        int16_t temp_raw = aht10_read_temperature_raw(); // exemplo: 2345 -> 23.45 C
        uint16_t hum_raw = aht10_read_humidity_raw();

        // montar pacote
        uint8_t pkt[10];
        pkt[0] = 0xAA;
        pkt[1] = 0x01; // version
        pkt[2] = NODE_ID;
        pkt[3] = (uint8_t)(seq & 0xFF);
        pkt[4] = (uint8_t)((seq >> 8) & 0xFF);
        pkt[5] = (uint8_t)((temp_raw >> 8) & 0xFF);
        pkt[6] = (uint8_t)(temp_raw & 0xFF);
        pkt[7] = (uint8_t)((hum_raw >> 8) & 0xFF);
        pkt[8] = (uint8_t)(hum_raw & 0xFF);
        pkt[9] = checksum(&pkt[1], 8);

        rfm96_send(pkt, 10);

        seq++;

        // esperar 10s
        timer_sleep_ms(10000);
    }
    return 0;
}
