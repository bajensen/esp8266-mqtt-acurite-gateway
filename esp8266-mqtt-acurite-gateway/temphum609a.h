#ifndef _TEMPHUM609A_H
#define _TEMPHUM609A_H

// ACU-RITE outdoor temperature sensor receiver
// FCC ID: RNE00609A2TX
// IC: 6608A-00609A2TX
// Model: 00609A2TX

// Format
// 5 bytes (40 bits)
// ######## ????TTTT TTTTTTTT HHHHHHHH ********
// # = ID Bits
// T = Temp Bits (celcius, signed)
// H = Humidity Bits (percent)
// * = Checksum?
// ? = unknown

#define TEMPHUM609A_NAME "temphum609a"

#define TEMPHUM609A_PREAMBLE_LEN  8900
#define TEMPHUM609A_PREAMBLE_TOL   100
#define TEMPHUM609A_PREAMBLE_MIN (TEMPHUM609A_PREAMBLE_LEN - TEMPHUM609A_PREAMBLE_TOL)
#define TEMPHUM609A_PREAMBLE_MAX (TEMPHUM609A_PREAMBLE_LEN + TEMPHUM609A_PREAMBLE_TOL)

#define TEMPHUM609A_POSTAMBLE_LEN 10937
#define TEMPHUM609A_POSTAMBLE_TOL   100
#define TEMPHUM609A_POSTAMBLE_MIN (TEMPHUM609A_POSTAMBLE_LEN - TEMPHUM609A_POSTAMBLE_TOL)
#define TEMPHUM609A_POSTAMBLE_MAX (TEMPHUM609A_POSTAMBLE_LEN + TEMPHUM609A_POSTAMBLE_TOL)

#define TEMPHUM609A_SEP_LEN 600
#define TEMPHUM609A_SEP_TOL  70
#define TEMPHUM609A_SEP_LMIN (TEMPHUM609A_SEP_LEN - TEMPHUM609A_SEP_TOL)
#define TEMPHUM609A_SEP_LMAX (TEMPHUM609A_SEP_LEN + TEMPHUM609A_SEP_TOL)

#define TEMPHUM609A_BIT1_LEN 1960
#define TEMPHUM609A_BIT1_TOL   50
#define TEMPHUM609A_BIT1_LMIN (TEMPHUM609A_BIT1_LEN - TEMPHUM609A_BIT1_TOL)
#define TEMPHUM609A_BIT1_LMAX (TEMPHUM609A_BIT1_LEN + TEMPHUM609A_BIT1_TOL)

#define TEMPHUM609A_BIT0_LEN  980
#define TEMPHUM609A_BIT0_TOL   50
#define TEMPHUM609A_BIT0_LMIN (TEMPHUM609A_BIT0_LEN - TEMPHUM609A_BIT0_TOL)
#define TEMPHUM609A_BIT0_LMAX (TEMPHUM609A_BIT0_LEN + TEMPHUM609A_BIT0_TOL)

#define TEMPHUM609A_NUM_CHANGES 88

#define TEMPHUM609A_IGNORE_REPEAT_MS 3000

typedef struct temphum609a_data_s {
  uint8_t id;
  int16_t temp;
  uint8_t hum;
  uint8_t checksum;
  unsigned long tsMs;
} temphum609a_data_t;

temphum609a_data_t temphum609a_data_prev;
temphum609a_data_t temphum609a_data_cur;

void temphum609a_init();
ICACHE_RAM_ATTR bool temphum609a_detect();
bool temphum609a_decode();
uint16_t temphum609a_getId();
float temphum609a_getTemp();
byte temphum609a_getHum();
void temphum609a_encode(char *buffer, unsigned int bufferSize);
bool temphum609a_shouldSend();
bool temphum609a_isNew();
void temphum609a_store();
inline byte temphum609a_getBitVal(unsigned int offset, unsigned int i, bool* good);


void temphum609a_init() {
  plugins_register(
    TEMPHUM609A_NAME,
    &temphum609a_detect, 
    &temphum609a_decode,
    &temphum609a_shouldSend,
    &temphum609a_getId,
    &temphum609a_encode
  );
}

ICACHE_RAM_ATTR bool temphum609a_detect() {
  unsigned int timingSize = timings.size();

  if (timingSize < (TEMPHUM609A_NUM_CHANGES + 1)) return false;

  return (
    inRange(timings[timingSize-1], TEMPHUM609A_SEP_LMIN, TEMPHUM609A_SEP_LMAX) &&
    inRange(timings[timingSize-2], TEMPHUM609A_POSTAMBLE_MIN, TEMPHUM609A_POSTAMBLE_MAX) &&
    inRange(timings[timingSize-TEMPHUM609A_NUM_CHANGES+4], TEMPHUM609A_PREAMBLE_MIN, TEMPHUM609A_PREAMBLE_MAX)
  );
}

bool temphum609a_decode () {
  unsigned int i;
  unsigned long dur;

  temphum609a_data_cur.id = 0;
  temphum609a_data_cur.temp = 0;
  temphum609a_data_cur.hum = 0;
  temphum609a_data_cur.checksum = 0;
  temphum609a_data_cur.tsMs = millis();

  bool good = true;

  unsigned int offset = timings.size() - TEMPHUM609A_NUM_CHANGES;

  // ID
  for (i = 0; i < 8; i++) {
    temphum609a_data_cur.id <<= 1;
    temphum609a_data_cur.id |= temphum609a_getBitVal(offset, i, &good);
  }

  // Temperature
  for (i = 12; i < 24; i++) {
    temphum609a_data_cur.temp <<= 1;
    temphum609a_data_cur.temp |= temphum609a_getBitVal(offset, i, &good);
  }
  temphum609a_data_cur.temp |= ((temphum609a_data_cur.temp&0x0800) ? (0xF000) : (0x0000)); // sign extend

  // Humidity
  for (i = 24; i < 32; i++) {
    temphum609a_data_cur.hum <<= 1;
    temphum609a_data_cur.hum |= temphum609a_getBitVal(offset, i, &good);
  }

  // Checksum
  for (i = 32; i < 40; i++) {
    temphum609a_data_cur.checksum <<= 1;
    temphum609a_data_cur.checksum |= temphum609a_getBitVal(offset, i, &good);
  }

  if (temphum609a_getTemp() < -40 || temphum609a_getTemp() > 70) {
    snprintf(msgBuffer, MSG_BUFFER_SIZE, TEMPHUM609A_NAME ": Out of range: temp=%f\n", temphum609a_getTemp());
    wifi_log(msgBuffer);
    good = false;
  }

  if (temphum609a_getHum() < 0 || temphum609a_getHum() > 100) {
    snprintf(msgBuffer, MSG_BUFFER_SIZE, TEMPHUM609A_NAME ": Out of range: hum=%d\n", temphum609a_getHum());
    wifi_log(msgBuffer);
    good = false;
  }

  return good;
}

uint16_t temphum609a_getId() {
  return temphum609a_data_cur.id;
}

float temphum609a_getTemp() {
  return (temphum609a_data_cur.temp / 10.0f);
}

byte temphum609a_getHum() {
  return temphum609a_data_cur.hum;
}

void temphum609a_encode(char *buffer, unsigned int bufferSize) {
  float temp = temphum609a_getTemp();
  snprintf(
    buffer, bufferSize, 
    "{\"temp\":{\"c\":%.1f,\"f\":%.2f},\"hum\":%d}",
    temp,
    c_to_f(temp),
    temphum609a_getHum()
  );
}

bool temphum609a_shouldSend() {
  bool isNew = temphum609a_isNew();
  temphum609a_store();
  return isNew;
}

bool temphum609a_isNew() {
  return (
    temphum609a_data_cur.id != temphum609a_data_prev.id ||
    temphum609a_data_cur.temp != temphum609a_data_prev.temp ||
    temphum609a_data_cur.hum != temphum609a_data_prev.hum ||
    temphum609a_data_cur.checksum != temphum609a_data_prev.checksum ||
    temphum609a_data_cur.tsMs >= temphum609a_data_prev.tsMs + TEMPHUM609A_IGNORE_REPEAT_MS
  );
}

void temphum609a_store() {
  temphum609a_data_prev.id = temphum609a_data_cur.id;
  temphum609a_data_prev.temp = temphum609a_data_cur.temp;
  temphum609a_data_prev.hum = temphum609a_data_cur.hum;
  temphum609a_data_prev.checksum = temphum609a_data_cur.checksum;
  temphum609a_data_prev.tsMs = temphum609a_data_cur.tsMs;
}

inline byte temphum609a_getBitVal(unsigned int offset, unsigned int i, bool* good) {  
  unsigned long dur = timings[offset + i*2 + 6];
  
  if (inRange(dur, TEMPHUM609A_BIT1_LMIN, TEMPHUM609A_BIT1_LMAX)) {
    return 1;
  }
  else if (inRange(dur, TEMPHUM609A_BIT0_LMIN, TEMPHUM609A_BIT0_LMAX)) {
    return 0;
  }
  else {
    *good = false;
  }
}

#endif
