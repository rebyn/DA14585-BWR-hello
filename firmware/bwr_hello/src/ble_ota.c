#include "ble_ota.h"
const uint8_t OTA_MAGIC[OTA_MAGIC_LEN] = {0x79,0x13,0xA5,0xF9,0x86,0xEC,0x5A,0x06};

void ota_header(uint8_t out[OTA_HDR_LEN], uint32_t size, uint32_t crc, uint32_t ver) {
    for (int i = 0; i < OTA_HDR_LEN; i++) out[i] = 0xFF;
    out[0]=0x70; out[1]=0x51; out[2]=0xAA; out[3]=0x00;  // 00AA5170 LE
    out[4]= size & 0xFF; out[5]=(size>>8)&0xFF; out[6]=(size>>16)&0xFF; out[7]=(size>>24)&0xFF;
    out[8]= crc & 0xFF;  out[9]=(crc>>8)&0xFF;  out[10]=(crc>>16)&0xFF; out[11]=(crc>>24)&0xFF;
    out[28]= ver & 0xFF; out[29]=(ver>>8)&0xFF; out[30]=(ver>>16)&0xFF; out[31]=(ver>>24)&0xFF;
    out[32]=0x00;
}

int ota_magic_find(const uint8_t *buf, int n) {
    for (int i = 0; i + OTA_MAGIC_LEN + 4 <= n; i++) {
        int j = 0;
        while (j < OTA_MAGIC_LEN && buf[i+j] == OTA_MAGIC[j]) j++;
        if (j == OTA_MAGIC_LEN) return i;
    }
    return -1;
}

uint32_t ota_crc(const uint8_t *buf, int n) {
    uint32_t c = 0xFFFFFFFF;
    for (int i = 0; i < n; i++) {
        c ^= buf[i];
        for (int k = 0; k < 8; k++) c = (c & 1) ? (c >> 1) ^ 0xEDB88320 : (c >> 1);
    }
    return c ^ 0xFFFFFFFF;
}
