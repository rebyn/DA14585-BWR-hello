#ifndef BLE_OTA_H_
#define BLE_OTA_H_
// Vendor EPD-family BLE surface (kept so the existing webtool flashes us):
//   svc  62750001-d828-918d-fb46-b6c11c675aec
//   char 62750002-... write+notify (commands + OTA), 62750003-... 1B version.
// Name: DCLK-xxxx (stock HMCLOCK convention; vendor webtool path dropped).
// OTA opcodes on ...0002 (write-with-response, 136B writes):
//   A0 + u32LE(firmSize)              erase inactive bank
//   A2 + 135B / A3 + 135B             256B page halves (first page: 64B hdr + 192B fw)
//   A4                                commit header, reboot into new bank
// Bank header (64B): 00AA5170 / u32 size / u32 crc32 / 0.. / u32 firmVer@28 / 0.
// File magic (else webtool refuses): 79 13 A5 F9 86 EC 5A 06 + u32LE firmVer.
// Asset blob (EPDA, 0x2C open/0x01 chunks/0x02 crc-close) is NOT firmware;
// hello needs none (device reports asset=none until pushed).
#include <stdint.h>
#include <stdbool.h>
#define OTA_MAGIC_LEN 8
extern const uint8_t OTA_MAGIC[OTA_MAGIC_LEN];
#define OTA_HDR_LEN 64
#define OTA_PAGE 256
// Pure helpers (host-tested): header build + page split + magic scan.
void ota_header(uint8_t out[OTA_HDR_LEN], uint32_t size, uint32_t crc, uint32_t ver);
int ota_magic_find(const uint8_t *buf, int n);  // offset of magic, -1 if none
uint32_t ota_crc(const uint8_t *buf, int n);    // IEEE crc32
#endif
