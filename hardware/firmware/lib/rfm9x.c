#include <stdio.h>
#include <string.h>
#include <generated/csr.h> 
#include <system.h>
#include <time.h>

#include "rfm9x.h" 

static uint8_t spi_transfer(uint8_t val) {
    spi_mosi_write(val);
    
    spi_control_write( (8 << CSR_SPI_CONTROL_LENGTH_OFFSET) | (1 << CSR_SPI_CONTROL_START_OFFSET) );
    
    while (!(spi_status_read() & (1 << CSR_SPI_STATUS_DONE_OFFSET)));
    
    return spi_miso_read();
}

void rfm9x_select(void) {
    spi_cs_write(0);
}

void rfm9x_deselect(void) {
    spi_cs_write(1);
}

uint8_t rfm9x_read(uint8_t reg) {
    uint8_t val;
    rfm9x_select();
    spi_transfer(reg & 0x7F);
    val = spi_transfer(0x00);
    rfm9x_deselect();
    return val;
}

void rfm9x_write(uint8_t reg, uint8_t val) {
    rfm9x_select();
    spi_transfer(reg | 0x80);
    spi_transfer(val);
    rfm9x_deselect();
}

void rfm9x_reset(void) {
    spi_rst_out_write(0);
    busy_wait_us(10000);
    spi_rst_out_write(1);
    busy_wait_us(10000);
}

void rfm9x_setup(uint64_t frequency) {
    rfm9x_deselect();
    spi_rst_out_write(1);

    rfm9x_reset();

    uint8_t version = rfm9x_read(REG_VERSION);
    if (version != 0x12) {
        printf("Falha ao encontrar o RFM9x, verifique a fiacao! Versao: 0x%02X\n", version);
        while(1);
    }
    printf("Versao do RFM9x encontrada: 0x%02X\n", version);

    rfm9x_write(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_SLEEP);
    
    uint64_t frf = (frequency << 19) / 32000000;
    rfm9x_write(REG_FRF_MSB, (frf >> 16) & 0xFF);
    rfm9x_write(REG_FRF_MID, (frf >> 8) & 0xFF);
    rfm9x_write(REG_FRF_LSB, frf & 0xFF);

    rfm9x_write(REG_FIFO_TX_BASE, 0x00);
    rfm9x_write(REG_FIFO_RX_BASE, 0x00);
    rfm9x_write(REG_PA_CONFIG, 0x8F);
    rfm9x_write(REG_MODEM_CONFIG_1, 0x72);
    rfm9x_write(REG_MODEM_CONFIG_2, 0x74);
    
    rfm9x_write(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_STDBY);
}

void rfm9x_send(const char* msg) {
    rfm9x_write(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_STDBY);
    rfm9x_write(REG_FIFO_ADDR_PTR, 0);
    
    int len = strlen(msg);
    for (int i = 0; i < len; i++) {
        rfm9x_write(REG_FIFO, msg[i]);
    }
    rfm9x_write(REG_PAYLOAD_LENGTH, len);
    
    rfm9x_write(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_TX);
    
    while (!(rfm9x_read(REG_IRQ_FLAGS) & IRQ_TX_DONE_MASK)) {
    }
    
    rfm9x_write(REG_IRQ_FLAGS, IRQ_TX_DONE_MASK);
}

int rfm9x_receive(char* buf, int max_len) {
    rfm9x_write(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_RX_CONTINUOUS);
    
    while (!(rfm9x_read(REG_IRQ_FLAGS) & IRQ_RX_DONE_MASK)) {
    }

    int len = rfm9x_read(REG_RX_NB_BYTES);
    rfm9x_write(REG_FIFO_ADDR_PTR, rfm9x_read(REG_FIFO_RX_CURRENT_ADDR));
    
    if (len > max_len - 1) len = max_len - 1;
    
    for (int i = 0; i < len; i++) {
        buf[i] = rfm9x_read(REG_FIFO);
    }
    buf[len] = '\0';
    
    rfm9x_write(REG_IRQ_FLAGS, IRQ_RX_DONE_MASK);
    return len;
}