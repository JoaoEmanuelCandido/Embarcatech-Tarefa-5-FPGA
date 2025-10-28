#ifndef RFM9X_H
#define RFM9X_H

#include <stdint.h>
#include <generated/csr.h>
#include <time.h>

#define REG_FIFO                    0x00
#define REG_OP_MODE                 0x01
#define REG_FRF_MSB                 0x06
#define REG_FRF_MID                 0x07
#define REG_FRF_LSB                 0x08
#define REG_PA_CONFIG               0x09
#define REG_PA_RAMP                 0x0A
#define REG_OCP                     0x0B
#define REG_LNA                     0x0C
#define REG_FIFO_ADDR_PTR           0x0D
#define REG_FIFO_TX_BASE            0x0E
#define REG_FIFO_RX_BASE            0x0F
#define REG_FIFO_RX_CURRENT_ADDR    0x10
#define REG_IRQ_FLAGS_MASK          0x11
#define REG_IRQ_FLAGS               0x12
#define REG_RX_NB_BYTES             0x13
#define REG_MODEM_CONFIG_1          0x1D
#define REG_MODEM_CONFIG_2          0x1E
#define REG_PAYLOAD_LENGTH          0x22
#define REG_VERSION                 0x42

#define MODE_LONG_RANGE_MODE        0x80
#define MODE_SLEEP                  0x00
#define MODE_STDBY                  0x01
#define MODE_TX                     0x03
#define MODE_RX_CONTINUOUS          0x05

#define IRQ_TX_DONE_MASK            0x08
#define IRQ_RX_DONE_MASK            0x40

void rfm9x_select(void);
void rfm9x_deselect(void);
uint8_t rfm9x_read(uint8_t reg);
void rfm9x_write(uint8_t reg, uint8_t val);
void rfm9x_reset(void);
void rfm9x_setup(uint64_t frequency);
void rfm9x_send(const char *msg);
int rfm9x_receive(char *buf, int max_len);

#endif