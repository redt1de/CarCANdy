#include "CANManager.h"
#include "FilterManager.h"
#include <string.h>

namespace {
CANManager::FrameCallback g_frameCallback = nullptr;
FilterManager *g_filterManager = nullptr;

// Total usable mailboxes when FIFO is enabled (FIFO consumes several
// mailbox slots internally for its own queue/filter table) -- matches the
// value used in prior bench-tested standalone sketches for this hardware.
constexpr uint8_t kMaxMailboxes = 16;

CarCANdy::CanFrame toCanFrame(uint8_t bus, const CAN_message_t &msg) {
  CarCANdy::CanFrame frame{};
  frame.bus = bus;
  frame.id = msg.id;
  frame.dlc = msg.len;
  frame.extended = msg.flags.extended;
  frame.rtr = msg.flags.remote;
  frame.timestampMs = millis();
  for (uint8_t i = 0; i < msg.len && i < 8; ++i) {
    frame.data[i] = msg.buf[i];
  }
  return frame;
}

CAN_message_t toCanMessage(const CarCANdy::CanFrame &frame) {
  CAN_message_t msg;
  msg.id = frame.id;
  msg.len = frame.dlc > 8 ? 8 : frame.dlc;
  msg.flags.extended = frame.extended;
  msg.flags.remote = frame.rtr;
  memcpy(msg.buf, frame.data, msg.len);
  return msg;
}

void dispatchFrame(uint8_t bus, const CAN_message_t &msg) {
  CarCANdy::CanFrame frame = toCanFrame(bus, msg);
  if (g_filterManager && !g_filterManager->shouldPass(frame.bus, frame.id)) {
    return;
  }
  if (g_frameCallback) {
    g_frameCallback(frame);
  }
}

// Bus numbers match the CAN controller (1=CAN1, 2=CAN2, 3=CAN3) -- see
// CANManager.h.
void onCan1Frame(const CAN_message_t &msg) { dispatchFrame(1, msg); }
void onCan2Frame(const CAN_message_t &msg) { dispatchFrame(2, msg); }
void onCan3Frame(const CAN_message_t &msg) { dispatchFrame(3, msg); }
} // namespace

CANManager::CANManager(ConfigManager &config, FilterManager &filterManager)
    : config_(config) {
  g_filterManager = &filterManager;
}

void CANManager::begin() {
  auto bus1 = config_.canBus(1);
  busEnabled_[1] = bus1.enabled;
  busTransmitAllowed_[1] = strcmp(bus1.mode, "transmit") == 0;
  Serial.printf("[CANManager] bus1 enabled=%d baud=%lu mode=%s\n",
                bus1.enabled, bus1.baud, bus1.mode);
  if (bus1.enabled) {
    can1_.begin();
    // LISTEN_ONLY unless the bus is configured for transmit: a normal (TX)
    // FlexCAN controller still tries to drive the mandatory ACK bit for
    // every frame it receives, even if the software never calls write().
    // If the transceiver's TX line isn't physically wired (e.g. read-only
    // bench setups), that ACK attempt never reaches the bus and shows up
    // as a BIT0 error on every reception. LISTEN_ONLY tells the controller
    // to never drive the bus at all, matching read-only wiring.
    can1_.setBaudRate(bus1.baud,
                       busTransmitAllowed_[1] ? TX : LISTEN_ONLY);
    // setMaxMB()+enableFIFO()+enableFIFOInterrupt() are required for RX to
    // actually work: begin()'s default (non-FIFO) mailbox setup only
    // enables interrupts on the TX half of mailboxes, so without FIFO mode
    // frames are accepted by the controller in hardware but the ISR never
    // notices them and onReceive() never fires. See DOC/open-risks.md.
    can1_.setMaxMB(kMaxMailboxes);
    can1_.enableFIFO();
    can1_.enableFIFOInterrupt();
    can1_.onReceive(onCan1Frame);
  }

  auto bus2 = config_.canBus(2);
  busEnabled_[2] = bus2.enabled;
  busTransmitAllowed_[2] = strcmp(bus2.mode, "transmit") == 0;
  Serial.printf("[CANManager] bus2 enabled=%d baud=%lu mode=%s\n",
                bus2.enabled, bus2.baud, bus2.mode);
  if (bus2.enabled) {
    can2_.begin();
    can2_.setBaudRate(bus2.baud,
                       busTransmitAllowed_[2] ? TX : LISTEN_ONLY);
    can2_.setMaxMB(kMaxMailboxes);
    can2_.enableFIFO();
    can2_.enableFIFOInterrupt();
    can2_.onReceive(onCan2Frame);
  }

  auto bus3 = config_.canBus(3);
  busEnabled_[3] = bus3.enabled;
  busTransmitAllowed_[3] = strcmp(bus3.mode, "transmit") == 0;
  Serial.printf("[CANManager] bus3 enabled=%d baud=%lu mode=%s\n",
                bus3.enabled, bus3.baud, bus3.mode);
  if (bus3.enabled) {
    can3_.begin();
    can3_.setBaudRate(bus3.baud,
                       busTransmitAllowed_[3] ? TX : LISTEN_ONLY);
    can3_.setMaxMB(kMaxMailboxes);
    can3_.enableFIFO();
    can3_.enableFIFOInterrupt();
    can3_.onReceive(onCan3Frame);
  }
}

void CANManager::loop() {
  can1_.events();
  can2_.events();
  can3_.events();

  static uint32_t lastDiagMs = 0;
  uint32_t now = millis();
  if (busEnabled_[1] && now - lastDiagMs >= 1000) {
    lastDiagMs = now;
    CAN_error_t err;
    can1_.error(err, false);
    Serial.printf("[CANManager] bus1 diag state=%s rx_err=%u tx_err=%u "
                  "rx_queue=%lu ACK=%d CRC=%d STF=%d FRM=%d BIT0=%d BIT1=%d\n",
                  err.state, err.RX_ERR_COUNTER, err.TX_ERR_COUNTER,
                  (unsigned long)can1_.getRXQueueCount(), err.ACK_ERR,
                  err.CRC_ERR, err.STF_ERR, err.FRM_ERR, err.BIT0_ERR,
                  err.BIT1_ERR);
  }
}

void CANManager::setFrameCallback(FrameCallback cb) {
  frameCallback_ = cb;
  g_frameCallback = cb;
}

bool CANManager::sendFrame(uint8_t bus, const CarCANdy::CanFrame &frame) {
  if (bus < 1 || bus > kBusCount || !busEnabled_[bus] || !busTransmitAllowed_[bus]) {
    return false;
  }

  CAN_message_t msg = toCanMessage(frame);
  switch (bus) {
  case 1:
    return can1_.write(msg) != 0;
  case 2:
    return can2_.write(msg) != 0;
  case 3:
    return can3_.write(msg) != 0;
  default:
    return false;
  }
}
