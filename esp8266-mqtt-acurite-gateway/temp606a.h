#ifndef _TEMP606A_H
#define _TEMP606A_H

#include "globals.h"

// ACU-RITE outdoor temperature sensor receiver
// Model: 00606TXA1
// FCC ID: RNE606TXA1
// IC: 6608A-606TXA1

// Format:
// 11 bits rolling code
//  1 bit battery status?
// 12 bits temperature (celcius, fixed point /10)
//  8 bits hash code

#define TEMP606A_NAME  "temp606a"

#define TEMP606A_SYNC_LEN   8500
#define TEMP606A_SYNC_TOL    100
#define TEMP606A_SYNC_LMIN (TEMP606A_SYNC_LEN - TEMP606A_SYNC_TOL)
#define TEMP606A_SYNC_LMAX (TEMP606A_SYNC_LEN + TEMP606A_SYNC_TOL)

#define TEMP606A_SEP_LEN 525
#define TEMP606A_SEP_TOL  70
#define TEMP606A_SEP_LMIN (TEMP606A_SEP_LEN - TEMP606A_SEP_TOL)
#define TEMP606A_SEP_LMAX (TEMP606A_SEP_LEN + TEMP606A_SEP_TOL)

#define TEMP606A_BIT1_LEN 3860
#define TEMP606A_BIT1_TOL   50
#define TEMP606A_BIT1_LMIN (TEMP606A_BIT1_LEN - TEMP606A_BIT1_TOL)
#define TEMP606A_BIT1_LMAX (TEMP606A_BIT1_LEN + TEMP606A_BIT1_TOL)

#define TEMP606A_BIT0_LEN 1910
#define TEMP606A_BIT0_TOL   50
#define TEMP606A_BIT0_LMIN (TEMP606A_BIT0_LEN - TEMP606A_BIT0_TOL)
#define TEMP606A_BIT0_LMAX (TEMP606A_BIT0_LEN + TEMP606A_BIT0_TOL)

#define TEMP606A_NUM_CHANGES 68

#define TEMP606A_IGNORE_REPEAT_MS 3000

typedef struct temp606a_data_s {
  uint8_t id;
  int16_t temp;
  uint8_t checksum;
  unsigned long tsMs;
} temp606a_data_t;

temp606a_data_t temp606a_data_prev;
temp606a_data_t temp606a_data_cur;

void temp606a_init();
ICACHE_RAM_ATTR bool temp606a_detect();
bool temp606a_decode();
uint16_t temp606a_getId();
float temp606a_getTemp();
void temp606a_encode(char *buffer, unsigned int bufferSize);
bool temp606a_shouldSend();
bool temp606a_isNew();
void temp606a_store();
inline byte temp606a_getBitVal(unsigned int offset, unsigned int i, bool* good);


void temp606a_init() {
  plugins_register(
    TEMP606A_NAME,
    &temp606a_detect, 
    &temp606a_decode,
    &temp606a_shouldSend,
    &temp606a_getId,
    &temp606a_encode
  );
}

ICACHE_RAM_ATTR bool temp606a_detect() {
  unsigned int timingSize = timings.size();

  if (timingSize < (TEMP606A_NUM_CHANGES + 1)) return false;

  return (
    // Sync at end
    inRange(timings[timingSize-1], TEMP606A_SYNC_LMIN, TEMP606A_SYNC_LMAX) &&
    // Sync at start
    inRange(timings[timingSize-TEMP606A_NUM_CHANGES-1], TEMP606A_SYNC_LMIN, TEMP606A_SYNC_LMAX)
  );
}

bool temp606a_decode() {
  temp606a_data_cur.id = 0;
  temp606a_data_cur.temp = 0;
  temp606a_data_cur.checksum = 0;
  temp606a_data_cur.tsMs = millis();

  bool good = true;

  unsigned int offset = timings.size() - TEMP606A_NUM_CHANGES;
  
  for (unsigned int i = 0; i < 12; i++) {
    temp606a_data_cur.id <<= 1;
    temp606a_data_cur.id |= temp606a_getBitVal(offset, i, &good);
  }
  
  for (unsigned int i = 12; i < 24; i++) {
    temp606a_data_cur.temp <<= 1;
    temp606a_data_cur.temp |= temp606a_getBitVal(offset, i, &good);
  }
  temp606a_data_cur.temp |= ((temp606a_data_cur.temp&0x0800) ? (0xF000) : (0x0000)); // sign extend
  
  for (unsigned int i = 24; i < 32; i++) {
    temp606a_data_cur.checksum <<= 1;
    temp606a_data_cur.checksum |= temp606a_getBitVal(offset, i, &good);
  }

  if (temp606a_getTemp() < -40 || temp606a_getTemp() > 70) {
    snprintf(msgBuffer, MSG_BUFFER_SIZE, TEMP606A_NAME ": Out of range: temp=%f\n",temp606a_getTemp());
    wifi_log(msgBuffer);
    good = false;
  }

  return good;
}

uint16_t temp606a_getId() {
  return temp606a_data_cur.id;
}

float temp606a_getTemp() {
  return (temp606a_data_cur.temp / 10.0f);
}

void temp606a_encode(char *buffer, unsigned int bufferSize) {
  float temp = temp606a_getTemp();
  snprintf(
    buffer, bufferSize, 
    "{\"temp\":{\"c\":%.1f,\"f\":%.2f}}",
    temp, c_to_f(temp)
  );
}

bool temp606a_shouldSend() {
  bool isNew = temp606a_isNew();
  temp606a_store();
  return isNew;
}

bool temp606a_isNew() {
  return (
    temp606a_data_cur.id != temp606a_data_prev.id ||
    temp606a_data_cur.temp != temp606a_data_prev.temp ||
    temp606a_data_cur.checksum != temp606a_data_prev.checksum ||
    temp606a_data_cur.tsMs >= temp606a_data_prev.tsMs + TEMP606A_IGNORE_REPEAT_MS
  );
}

void temp606a_store() {
  temp606a_data_prev.id = temp606a_data_cur.id;
  temp606a_data_prev.temp = temp606a_data_cur.temp;
  temp606a_data_prev.checksum = temp606a_data_cur.checksum;
  temp606a_data_prev.tsMs = temp606a_data_cur.tsMs;
}

inline byte temp606a_getBitVal(unsigned int offset, unsigned int i, bool* good) {  
  unsigned long dur = timings[offset + i*2 + 1];
  
  if (inRange(dur, TEMP606A_BIT1_LMIN, TEMP606A_BIT1_LMAX)) {
    return 1;
  }
  else if (inRange(dur, TEMP606A_BIT0_LMIN, TEMP606A_BIT0_LMAX)) {
    return 0;
  }
  else {
    *good = false;
  }
}

#endif
