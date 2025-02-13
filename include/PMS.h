#ifndef PMS_H
#define PMS_H

#include <Stream.h>

class PMS
{
public:
  struct DATA {
    // Standard Particles, CF=1
    uint16_t PM_SP_UG_1_0;
    uint16_t PM_SP_UG_2_5;
    uint16_t PM_SP_UG_10_0;

    // Atmospheric environment
    uint16_t PM_AE_UG_1_0;
    uint16_t PM_AE_UG_2_5;
    uint16_t PM_AE_UG_10_0;

    // Number of particles
    uint16_t NUM_UM_0_3;
    uint16_t NUM_UM_0_5;
    uint16_t NUM_UM_1_0;
    uint16_t NUM_UM_2_5;
    uint16_t NUM_UM_5_0;
    uint16_t NUM_UM_10_0;
  };

  PMS(Stream&);
  void sleep();
  void wakeUp();
  void activeMode();
  void passiveMode();

  void requestRead();
  bool read(DATA *data);
  bool readUntil(DATA *data, uint16_t timeout = 1000);

  static float altCF3(PMS::DATA& data);
  static float epa(float x, float rh);

private:
  enum MODE { MODE_ACTIVE, MODE_PASSIVE };

  uint8_t _payload[24];
  Stream* _stream;
  MODE _mode = MODE_ACTIVE;

  uint8_t _index = 0;
  uint16_t _frameLen;
  uint16_t _checksum;
  uint16_t _calculatedChecksum;
};

#endif