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
  if (bus1.enabled) {
    can1_.begin();
    can1_.setBaudRate(bus1.baud);
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
  if (bus2.enabled) {
    can2_.begin();
    can2_.setBaudRate(bus2.baud);
    can2_.setMaxMB(kMaxMailboxes);
    can2_.enableFIFO();
    can2_.enableFIFOInterrupt();
    can2_.onReceive(onCan2Frame);
  }

  auto bus3 = config_.canBus(3);
  busEnabled_[3] = bus3.enabled;
  busTransmitAllowed_[3] = strcmp(bus3.mode, "transmit") == 0;
  if (bus3.enabled) {
    can3_.begin();
    can3_.setBaudRate(bus3.baud);
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
