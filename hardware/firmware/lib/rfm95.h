// ./lib/rfm95.h
#pragma once
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

uint8_t rfm95_read_reg(uint8_t reg);
void    rfm95_write_reg(uint8_t reg, uint8_t value);
void    rfm95_set_mode(uint8_t mode);
bool    rfm95_init(void);
bool    rfm95_send_bytes(const uint8_t *data, size_t len);
