#pragma once
#include "AuxPorts.h"
#include "CANManager.h"
#include "ConfigManager.h"
#include "PluginBase.h"
#include <ArduinoJson.h>
#include <math.h>
#include <string.h>

// Broadcasts vehicle data to the gauges add-on (AddOns/Gauges) over a
// one-wire UART link from one of the AUX ports' TX pin. All 3 gauge
// displays listen on the same wire and each picks the one field its own
// build is showing -- see AddOns/Gauges/src/live_data.cpp.
//
// Wire format: one ASCII line per send, comma-separated NAME:VALUE pairs,
// terminated with '\n'. NAME matches the GaugeSpec.name strings in
// AddOns/Gauges/src/gauge_specs.cpp exactly, and VALUE is an ASCII float
// already converted into that gauge's own display unit (so the gauge side
// treats it identically to its own simulated data -- no unit math on
// receive). "nan" is sent for any signal whose CAN ID/formula isn't known
// yet, see the per-signal TODOs in onCanFrame() below. e.g.:
//   BOOST:nan,AFR:nan,COOLANT:nan,RPM:3252.0,TRANS:nan,SPEED:nan,VOLTAGE:nan,OIL:nan
class GaugesPlugin : public PluginBase {
public:
  const char *getName() const override { return "gauges"; }

  void setDefaultConfig(JsonObject cfg) const override {
    // ShiftLightPlugin defaults to AUX2 (its neopixel strip), so this
    // defaults to AUX1 instead to avoid a collision when both plugins are
    // enabled with their defaults. "AUX1" or "AUX2".
    setDefault(cfg, "auxPort", "AUX1");
    setDefault(cfg, "intervalMs", 100);
  }

  void init(ConfigManager &config, CANManager &canManager, Logging &logging) override {
    (void)canManager;
    (void)logging;
    JsonObjectConst cfg = config.pluginConfig(getName());
    intervalMs_ = cfg["intervalMs"];
    const bool useAux2 = strcmp(cfg["auxPort"] | "AUX1", "AUX2") == 0;
    serial_ = useAux2 ? &AuxPorts::AUX2_SERIAL : &AuxPorts::AUX1_SERIAL;
    serial_->begin(kBaudRate);
  }

  void loop() override {
    const unsigned long now = millis();
    if (now - lastSendMs_ < intervalMs_) {
      return;
    }
    lastSendMs_ = now;
    send();
  }

  void onCanFrame(const CarCANdy::CanFrame &frame) override {
    // RPM -- confirmed on real hardware, reuses ShiftLightPlugin's decode.
    if (frame.bus == 1 && frame.id == 0x204) {
      readings_[GAUGE_RPM] = (frame.data[3] * 256 + frame.data[4]) * 2;
    }

    // TODO(user): the remaining 7 signals' CAN IDs/formulas aren't known
    // yet. Add a case here the same way RPM is handled above -- write the
    // value, already converted into the unit shown in
    // AddOns/Gauges/src/gauge_specs.cpp, into readings_[GAUGE_*]:
    //   GAUGE_BOOST    PSI
    //   GAUGE_AFR      air/fuel ratio
    //   GAUGE_COOLANT  degF
    //   GAUGE_TRANS    degF
    //   GAUGE_SPEED    MPH
    //   GAUGE_VOLTAGE  volts
    //   GAUGE_OIL      PSI (oil pressure -- DOC/CANDB only has oil_temp
    //                  (0x156) so far, not pressure)
    // Until then these stay NAN ("nan" on the wire), which the gauge side
    // holds at its last-known value -- see live_data.cpp.
  }

private:
  // Mirrors AddOns/Gauges/src/gauge.h's GaugeType enum order and
  // AddOns/Gauges/src/gauge_specs.cpp's GaugeSpec.name strings exactly --
  // the two projects are separate PlatformIO projects and don't share
  // headers, so this is the contract between them. Keep both in sync if
  // either changes.
  enum GaugeIndex {
    GAUGE_BOOST,
    GAUGE_AFR,
    GAUGE_COOLANT,
    GAUGE_RPM,
    GAUGE_TRANS,
    GAUGE_SPEED,
    GAUGE_VOLTAGE,
    GAUGE_OIL,
    GAUGE_COUNT,
  };
  static constexpr const char *kNames[GAUGE_COUNT] = {
      "BOOST", "AFR", "COOLANT", "RPM", "TRANS", "SPEED", "VOLTAGE", "OIL"};
  static constexpr uint32_t kBaudRate = 115200;

  void send() {
    char line[160];
    int offset = 0;
    for (int i = 0; i < GAUGE_COUNT && offset < (int)sizeof(line); ++i) {
      offset += snprintf(line + offset, sizeof(line) - offset, "%s%s:%.1f",
                          i == 0 ? "" : ",", kNames[i], readings_[i]);
    }
    serial_->print(line);
    serial_->print('\n');
  }

  HardwareSerial *serial_ = nullptr;
  uint32_t intervalMs_ = 100;
  unsigned long lastSendMs_ = 0;
  float readings_[GAUGE_COUNT] = {NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN};
};
