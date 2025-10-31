// ==== rfm95.c (driver corrigido) ====
#include "./rfm95.h"

#include <stdio.h>
#include <string.h>
#include <generated/csr.h>
#include <system.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#define TX_TIMEOUT_MS 5000

#define SPI_MODE_MANUAL (1 << 16)
#define SPI_CS_MASK     0x0001

#define REG_FIFO                 0x00
#define REG_OP_MODE              0x01
#define REG_FRF_MSB              0x06
#define REG_FRF_MID              0x07
#define REG_FRF_LSB              0x08
#define REG_PA_CONFIG            0x09
#define REG_LNA                  0x0C
#define REG_FIFO_ADDR_PTR        0x0D
#define REG_FIFO_TX_BASE_ADDR    0x0E
#define REG_FIFO_RX_BASE_ADDR    0x0F
#define REG_IRQ_FLAGS_MASK       0x11
#define REG_IRQ_FLAGS            0x12
#define REG_MODEM_CONFIG_1       0x1D
#define REG_MODEM_CONFIG_2       0x1E
#define REG_PREAMBLE_MSB         0x20
#define REG_PREAMBLE_LSB         0x21
#define REG_PAYLOAD_LENGTH       0x22
#define REG_MODEM_CONFIG_3       0x26
#define REG_SYNC_WORD            0x39
#define REG_DIO_MAPPING_1        0x40
#define REG_VERSION              0x42
#define REG_PA_DAC               0x4D
#define REG_OCP                  0x0B

#define MODE_SLEEP               0x00
#define MODE_STDBY               0x01
#define MODE_TX                  0x03

#define IRQ_TX_DONE_MASK         0x08

static void busy_wait_ms_local(unsigned int ms);
static void spi_init(void);
static inline void rfm95_select(void);
static inline void rfm95_deselect(void);
static inline uint8_t rfm95_txrx(uint8_t tx_byte);
static void rfm95_write_fifo(const uint8_t *data, uint8_t len);

static void busy_wait_ms_local(unsigned int ms) {
    for (unsigned int i = 0; i < ms; ++i) {
#ifdef CSR_TIMER0_BASE
        busy_wait_us(1000);
#else
        for(volatile int j = 0; j < 2000; j++);
#endif
    }
}

static void spi_init(void) {
    spi_cs_write(SPI_MODE_MANUAL | 0x0000);
#ifdef CSR_SPI_LOOPBACK_ADDR
    spi_loopback_write(0);
#endif
    busy_wait_ms_local(1);
}

static inline void rfm95_select(void) {
    spi_cs_write(SPI_MODE_MANUAL | SPI_CS_MASK);
    busy_wait_us(2);
}

static inline void rfm95_deselect(void) {
    spi_cs_write(SPI_MODE_MANUAL | 0x0000);
    busy_wait_us(2);
}

static inline uint8_t rfm95_txrx(uint8_t tx_byte) {
    uint32_t rx_byte;
    spi_mosi_write((uint32_t)tx_byte);
    spi_control_write(
        (1 << CSR_SPI_CONTROL_START_OFFSET) |
        (8 << CSR_SPI_CONTROL_LENGTH_OFFSET)
    );
    while ((spi_status_read() & (1 << CSR_SPI_STATUS_DONE_OFFSET)) == 0) { }
    rx_byte = spi_miso_read();
    return (uint8_t)(rx_byte & 0xFF);
}

static void rfm95_write_fifo(const uint8_t *data, uint8_t len) {
    rfm95_select();
    rfm95_txrx(REG_FIFO | 0x80);
    for (uint8_t i = 0; i < len; i++) {
        rfm95_txrx(data[i]);
    }
    rfm95_deselect();
}

/* ===== API ===== */

uint8_t rfm95_read_reg(uint8_t reg) {
    uint8_t val;
    rfm95_select();
    rfm95_txrx(reg & 0x7F);
    val = rfm95_txrx(0x00);
    rfm95_deselect();
    return val;
}

void rfm95_write_reg(uint8_t reg, uint8_t value) {
    rfm95_select();
    rfm95_txrx(reg | 0x80);
    rfm95_txrx(value);
    rfm95_deselect();
}

void rfm95_set_mode(uint8_t mode) {
    rfm95_write_reg(REG_OP_MODE, (0x80 | mode));
}

bool rfm95_init(void) {
    spi_init();

#ifdef CSR_LORA_RESET_BASE
    lora_reset_out_write(0); busy_wait_ms_local(5);
    lora_reset_out_write(1); busy_wait_ms_local(10);
#endif

    uint8_t rx = rfm95_read_reg(REG_VERSION);
    if (rx != 0x12) {
        printf("⚠️ Versão inesperada (0x%02X, esperado 0x12). SPI falhou ou chip incorreto.\n", rx);
        return false;
    }

    rfm95_set_mode(MODE_SLEEP);

    /* 915 MHz  (FRF = Freq * 2^19 / 32e6) */
    uint64_t frf = ((uint64_t)915000000 << 19) / 32000000;
    rfm95_write_reg(REG_FRF_MSB, (uint8_t)(frf >> 16));
    rfm95_write_reg(REG_FRF_MID, (uint8_t)(frf >> 8));
    rfm95_write_reg(REG_FRF_LSB, (uint8_t)(frf >> 0));
    printf("Frequencia LoRa configurada para 915 MHz\n");

    /* Parametrização básica */
    rfm95_write_reg(REG_PA_CONFIG, 0xFF);
    rfm95_write_reg(REG_PA_DAC,    0x87);
    rfm95_write_reg(REG_MODEM_CONFIG_1, 0x78);
    rfm95_write_reg(REG_MODEM_CONFIG_2, 0xC4);
    rfm95_write_reg(REG_MODEM_CONFIG_3, 0x0C);
    rfm95_write_reg(REG_PREAMBLE_MSB,   0x00);
    rfm95_write_reg(REG_PREAMBLE_LSB,   0x0C);
    rfm95_write_reg(REG_SYNC_WORD,      0x12);
    rfm95_write_reg(REG_OCP,            0x37);
    rfm95_write_reg(REG_FIFO_TX_BASE_ADDR, 0x00);
    rfm95_write_reg(REG_FIFO_RX_BASE_ADDR, 0x00);
    rfm95_write_reg(REG_LNA, 0x23);
    rfm95_write_reg(REG_IRQ_FLAGS_MASK, 0x00);
    rfm95_write_reg(REG_IRQ_FLAGS,      0xFF);

    rfm95_set_mode(MODE_STDBY);
    busy_wait_ms_local(10);

    printf("Modulacao: BW=62.5kHz, SF=12, CR=4/8, Preamble=12, SyncWord=0x12\n");
    return true;
}

bool rfm95_send_bytes(const uint8_t *data, size_t len) {
    if (len == 0 || len > 255) {
        printf("Erro LoRa: Tamanho do pacote inválido (%d bytes)\n", (int)len);
        return false;
    }

    rfm95_set_mode(MODE_STDBY);

    rfm95_write_reg(REG_FIFO_ADDR_PTR, 0x00);
    rfm95_write_fifo(data, (uint8_t)len);
    rfm95_write_reg(REG_PAYLOAD_LENGTH, (uint8_t)len);

    rfm95_write_reg(REG_IRQ_FLAGS, 0xFF);
    rfm95_write_reg(REG_DIO_MAPPING_1, 0x40);

    printf("Enviando %d bytes via LoRa...\n", (int)len);

    rfm95_set_mode(MODE_TX);

    int timeout_cnt = TX_TIMEOUT_MS;
    while (timeout_cnt > 0) {
        if (rfm95_read_reg(REG_IRQ_FLAGS) & IRQ_TX_DONE_MASK) {
            rfm95_write_reg(REG_IRQ_FLAGS, IRQ_TX_DONE_MASK);
            rfm95_set_mode(MODE_STDBY);
            printf("Pacote enviado com sucesso!\n");
            return true;
        }
        busy_wait_ms_local(1);
        timeout_cnt--;
    }

    printf("Erro: Timeout de TX! O radio foi resetado para Standby.\n");
    rfm95_set_mode(MODE_STDBY);
    return false;
}
