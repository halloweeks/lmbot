#ifndef DES_H
#define DES_H

#include <stdint.h>

// global key (defined in des.c)
extern const uint8_t ENCRYPTION_KEY[8];

// API
void EncryptData(uint8_t *data, uint32_t size, uint8_t *out, const uint8_t *key);
void DecryptData(uint8_t *data, uint32_t size, uint8_t *out, const uint8_t *key);

#endif