#ifndef _TEMPHUM592TXR_H
#define _TEMPHUM592TXR_H

// ACU-RITE outdoor temperature sensor receiver
// FCC ID: RNERF100VTXR
// IC: 6608A-RF100VTXR
// Model: 00592TXR

// Format
// 7 bytes (48 bits)
// CCCCCCCC IIIIIIII SSSSSSSS PHHHHHHH PTTTTTTT PTTTTTTT XXXXXXXX
// C = Channel
// I = ID
// S = Battery Status
// P = Even Parity Bit for next 7 bits (0 = Even, 1 = Odd)
// H = Relative Humidity (7 bits percent)
// T = Temperature (14 bits, divide by 10 and subtract 100)
// X = Checksum (Sum of previous 6 bytes, modulo 256)

#define TEMPHUM592TXR_NAME  "temphum592txr"

#define TEMPHUM592TXR_PREAMBLE_LEN  600
#define TEMPHUM592TXR_PREAMBLE_TOL   20
#define TEMPHUM592TXR_PREAMBLE_MIN (TEMPHUM592TXR_PREAMBLE_LEN - TEMPHUM592TXR_PREAMBLE_TOL)
#define TEMPHUM592TXR_PREAMBLE_MAX (TEMPHUM592TXR_PREAMBLE_LEN + TEMPHUM592TXR_PREAMBLE_TOL)

#define TEMPHUM592TXR_BIT1_LEN 400
#define TEMPHUM592TXR_BIT1_TOL  20
#define TEMPHUM592TXR_BIT1_LMIN (TEMPHUM592TXR_BIT1_LEN - TEMPHUM592TXR_BIT1_TOL)
#define TEMPHUM592TXR_BIT1_LMAX (TEMPHUM592TXR_BIT1_LEN + TEMPHUM592TXR_BIT1_TOL)

#define TEMPHUM592TXR_BIT0_LEN 210
#define TEMPHUM592TXR_BIT0_TOL  20
#define TEMPHUM592TXR_BIT0_LMIN (TEMPHUM592TXR_BIT0_LEN - TEMPHUM592TXR_BIT0_TOL)
#define TEMPHUM592TXR_BIT0_LMAX (TEMPHUM592TXR_BIT0_LEN + TEMPHUM592TXR_BIT0_TOL)

#define TEMPHUM592TXR_NUM_CHANGES 122

#define TEMPHUM592TXR_IGNORE_REPEAT_MS 3000

typedef struct temphum592txr_data_s {
  uint8_t chan;
  uint8_t id;
  uint8_t bat;
  int16_t temp;
  uint8_t hum;
  uint8_t checksum;
  unsigned long tsMs;
} temphum592txr_data_t;

temphum592txr_data_t temphum592txr_data_prev;
temphum592txr_data_t temphum592txr_data_cur;

void temphum592txr_init();
ICACHE_RAM_ATTR bool temphum592txr_detect();
bool temphum592txr_decode();
uint16_t temphum592txr_getId();
float temphum592txr_getTemp();
byte temphum592txr_getHum();
byte temphum592txr_getLowBatt();
void temphum592txr_encode(char *buffer, unsigned int bufferSize);
bool temphum592txr_shouldSend();
bool temphum592txr_isNew();
void temphum592txr_store();
inline byte temphum592txr_getBitVal(unsigned int offset, unsigned int i, bool* good);


void temphum592txr_init() {
  plugins_register(
    TEMPHUM592TXR_NAME,
    &temphum592txr_detect, 
    &temphum592txr_decode,
    &temphum592txr_shouldSend,
    &temphum592txr_getId,
    &temphum592txr_encode
  );
}

ICACHE_RAM_ATTR bool temphum592txr_detect() {
  unsigned int timingSize = timings.size();
  
  if (timingSize < (TEMPHUM592TXR_NUM_CHANGES + 1)) return false;
  
  unsigned int startOffset = timingSize - TEMPHUM592TXR_NUM_CHANGES - 1;

  return (
    //// "0" postamble
    //inRange(timings[timingSize-1], TEMPHUM592TXR_BIT0_LMIN, TEMPHUM592TXR_BIT0_LMAX) &&
    // Preamble
    inRange(timings[startOffset+0], TEMPHUM592TXR_PREAMBLE_MIN, TEMPHUM592TXR_PREAMBLE_MAX) &&
    inRange(timings[startOffset+1], TEMPHUM592TXR_PREAMBLE_MIN, TEMPHUM592TXR_PREAMBLE_MAX) &&
    inRange(timings[startOffset+2], TEMPHUM592TXR_PREAMBLE_MIN, TEMPHUM592TXR_PREAMBLE_MAX) &&
    inRange(timings[startOffset+3], TEMPHUM592TXR_PREAMBLE_MIN, TEMPHUM592TXR_PREAMBLE_MAX) &&
    inRange(timings[startOffset+4], TEMPHUM592TXR_PREAMBLE_MIN, TEMPHUM592TXR_PREAMBLE_MAX) &&
    inRange(timings[startOffset+5], TEMPHUM592TXR_PREAMBLE_MIN, TEMPHUM592TXR_PREAMBLE_MAX) &&
    inRange(timings[startOffset+6], TEMPHUM592TXR_PREAMBLE_MIN, TEMPHUM592TXR_PREAMBLE_MAX) &&
    inRange(timings[startOffset+7], TEMPHUM592TXR_PREAMBLE_MIN, TEMPHUM592TXR_PREAMBLE_MAX)
  );
}

bool temphum592txr_decode () {
  unsigned int i;
  unsigned long dur;
  uint8_t calcParity;
  uint8_t calcChecksum;
  uint8_t tmp;

  bool good = true;

  unsigned int offset = timings.size() - TEMPHUM592TXR_NUM_CHANGES - 1;

  temphum592txr_data_cur.chan = 0;
  temphum592txr_data_cur.id = 0;
  temphum592txr_data_cur.temp = 0;
  temphum592txr_data_cur.hum = 0;
  temphum592txr_data_cur.checksum = 0;
  temphum592txr_data_cur.tsMs = millis();

  // Channel
  for (i = 0; i < 8; i++) {
    temphum592txr_data_cur.chan <<= 1;
    temphum592txr_data_cur.chan |= temphum592txr_getBitVal(offset, i, &good);
  }


  // ID
  for (i = 8; i < 16; i++) {
    temphum592txr_data_cur.id <<= 1;
    temphum592txr_data_cur.id |= temphum592txr_getBitVal(offset, i, &good);
  }

  // Batt
  for (i = 16; i < 24; i++) {
    temphum592txr_data_cur.bat <<= 1;
    temphum592txr_data_cur.bat |= temphum592txr_getBitVal(offset, i, &good);
  }

  // Humidity
  calcParity = 0;
  for (i = 25; i < 32; i++) {
    temphum592txr_data_cur.hum <<= 1;
    if (temphum592txr_getBitVal(offset, i, &good)) {
      temphum592txr_data_cur.hum |= 1;
      calcParity = ! calcParity;
    }
  }
  uint8_t humParity = temphum592txr_getBitVal(offset, 24, &good);
  if (humParity != calcParity) good = false;

  // Temperature
  // First 7 bits
  calcParity = 0;
  for (i = 33; i < 40; i++) {
    temphum592txr_data_cur.temp <<= 1;
    if (temphum592txr_getBitVal(offset, i, &good)) {
      temphum592txr_data_cur.temp |= 1;
      calcParity = ! calcParity;
    }
  }
  uint8_t tempParityUpper = temphum592txr_getBitVal(offset, 32, &good);
  if (tempParityUpper != calcParity) good = false;
  
  // Second 7 bits
  calcParity = 0;
  for (i = 41; i < 48; i++) {
    temphum592txr_data_cur.temp <<= 1;
    if (temphum592txr_getBitVal(offset, i, &good)) {
      temphum592txr_data_cur.temp |= 1;
      calcParity = ! calcParity;
    }
  }
  uint8_t tempParityLower = temphum592txr_getBitVal(offset, 40, &good);
  if (tempParityLower != calcParity) good = false;
  
  // Calculation
  temphum592txr_data_cur.temp |= ((temphum592txr_data_cur.temp & 0x2000) ? (0xC000) : (0x0000)); // sign extend

  // Checksum
  for (i = 48; i < 56; i++) {
    temphum592txr_data_cur.checksum <<= 1;
    temphum592txr_data_cur.checksum |= temphum592txr_getBitVal(offset, i, &good);
  }

  // Calculate checksum
  calcChecksum = 0;
  tmp = 0;
  // Yes, i is one greater than it should be... I don't want to handle the edge case :)
  for (i = 0; i < 48; i++) { 
    tmp <<= 1;
    tmp |= temphum592txr_getBitVal(offset, i, &good);
    
    if (i % 8 == 7) {
      calcChecksum += tmp;
      tmp = 0;
    }
  }

  // Check it
  if (calcChecksum != temphum592txr_data_cur.checksum) {
    snprintf(msgBuffer, MSG_BUFFER_SIZE, TEMPHUM592TXR_NAME ": Checksum mismatch: calc=%d, given=%d\n", calcChecksum, temphum592txr_data_cur.checksum);
    wifi_log(msgBuffer);
    return false;
  }

  if (temphum592txr_getTemp() < -40 || temphum592txr_getTemp() > 70) {
    snprintf(msgBuffer, MSG_BUFFER_SIZE, TEMPHUM592TXR_NAME ": Out of range: temp=%f\n", temphum592txr_getTemp());
    wifi_log(msgBuffer);
    return false;
  }

  if (temphum592txr_getHum() < 0 || temphum592txr_getHum() > 100) {
    snprintf(msgBuffer, MSG_BUFFER_SIZE, TEMPHUM592TXR_NAME ": Out of range: hum=%d\n", temphum592txr_getHum());
    wifi_log(msgBuffer);
    return false;
  }

  return good;
}

uint16_t temphum592txr_getId() {
  return (temphum592txr_data_cur.chan << 8) | temphum592txr_data_cur.id;
}

float temphum592txr_getTemp() {
  return (temphum592txr_data_cur.temp / 10.0f - 100.0f);
}

byte temphum592txr_getHum() {
  return temphum592txr_data_cur.hum;
}

byte temphum592txr_getLowBatt() {
  return (temphum592txr_data_cur.bat == 0x84);
}

void temphum592txr_encode(char *buffer, unsigned int bufferSize) {
  float temp = temphum592txr_getTemp();
  snprintf(
    buffer, bufferSize, 
    "{\"temp\":{\"c\":%.1f,\"f\":%.2f},\"hum\":%d,\"lowbatt\":%d}",
    temp,
    c_to_f(temp),
    temphum592txr_getHum(),
    temphum592txr_getLowBatt()
  );
}

bool temphum592txr_shouldSend() {
  bool isNew = temphum592txr_isNew();
  temphum592txr_store();
  return isNew;
}

bool temphum592txr_isNew() {
  return (
    temphum592txr_data_cur.chan != temphum592txr_data_prev.chan ||
    temphum592txr_data_cur.id != temphum592txr_data_prev.id ||
    temphum592txr_data_cur.temp != temphum592txr_data_prev.temp ||
    temphum592txr_data_cur.hum != temphum592txr_data_prev.hum ||
    temphum592txr_data_cur.checksum != temphum592txr_data_prev.checksum ||
    temphum592txr_data_cur.tsMs >= temphum592txr_data_prev.tsMs + TEMPHUM592TXR_IGNORE_REPEAT_MS
  );
}

void temphum592txr_store() {
  temphum592txr_data_prev.chan = temphum592txr_data_cur.chan;
  temphum592txr_data_prev.id = temphum592txr_data_cur.id;
  temphum592txr_data_prev.temp = temphum592txr_data_cur.temp;
  temphum592txr_data_prev.hum = temphum592txr_data_cur.hum;
  temphum592txr_data_prev.checksum = temphum592txr_data_cur.checksum;
  temphum592txr_data_prev.tsMs = temphum592txr_data_cur.tsMs;
}

inline byte temphum592txr_getBitVal(unsigned int offset, unsigned int i, bool* good) {  
  unsigned long dur = timings[offset + i*2 + 8];
  
  if (inRange(dur, TEMPHUM592TXR_BIT1_LMIN, TEMPHUM592TXR_BIT1_LMAX)) {
    return 1;
  }
  else if (inRange(dur, TEMPHUM592TXR_BIT0_LMIN, TEMPHUM592TXR_BIT0_LMAX)) {
    return 0;
  }
  else {
    *good = false;
  }
}

#endif
