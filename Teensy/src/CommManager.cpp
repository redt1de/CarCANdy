#include "CommManager.h"
#include "CANManager.h"
#include "FilterManager.h"
#include <Arduino.h>
#include <string.h>

using namespace CarCANdy;

CommManager::CommManager(ConfigManager& config, CANManager& canManager, FilterManager& filterManager)
    : config_(config), canManager_(canManager), filterManager_(filterManager) {}

void CommManager::begin() {
    bool useSpi = strcmp(config_.commMode(), "SPI") == 0;
    client_ = useSpi ? static_cast<CommClient*>(&spiClient_) : static_cast<CommClient*>(&uartClient_);

    if (!client_->begin()) {
        Serial.println("[CommManager] selected client failed to begin()");
    } else {
        Serial.printf("[CommManager] comm client started (mode=%s)\n", config_.commMode());
    }
}

void CommManager::poll() {
    if (!client_) {
        return;
    }
    client_->poll();

    Packet incoming;
    while (client_->readPacket(incoming)) {
        handlePacket(incoming);
    }
}

void CommManager::handlePacket(const Packet& pkt) {
    switch (static_cast<MsgType>(pkt.header.type)) {
    case MsgType::PING:
        sendAck();
        break;

    case MsgType::CONFIG_GET:
        Serial.println("[CommManager] CONFIG_GET received");
        sendConfigData();
        break;

    case MsgType::CONFIG_SET:
        Serial.println("[CommManager] CONFIG_SET received");
        if (config_.applyJson(pkt.payload, pkt.header.length)) {
            sendAck();
        } else {
            sendNack();
        }
        break;

    case MsgType::CAN_FRAME: {
        if (pkt.header.length != sizeof(CanFrame)) {
            sendNack();
            break;
        }
        CanFrame frame;
        memcpy(&frame, pkt.payload, sizeof(CanFrame));
        if (canManager_.sendFrame(frame.bus, frame)) {
            sendAck();
        } else {
            sendNack();
        }
        break;
    }

    case MsgType::CMD: {
        char cmd[80];
        const size_t len = pkt.header.length < sizeof(cmd) - 1 ? pkt.header.length : sizeof(cmd) - 1;
        memcpy(cmd, pkt.payload, len);
        cmd[len] = '\0';
        if (handleCmd(cmd)) {
            sendAck();
        } else {
            sendNack();
        }
        break;
    }

    default:
        Serial.printf("[CommManager] unhandled inbound MsgType 0x%02X\n", pkt.header.type);
        sendNack();
        break;
    }
}

// CMD grammar (ASCII payload, case-sensitive) -- see DOC/protocol.md:
//   RELOAD_FILTERS              -- re-read the filter CSV from SD
//   SET_FILTER_OVERRIDE:<mode>  -- mode = allow|deny|none|clear (clear
//                                   reverts to the configured mode); a
//                                   runtime-only override, not persisted
//   ADD_FILTER:<bus>,<id>[-<id>]    -- add + persist a filter entry
//   REMOVE_FILTER:<bus>,<id>[-<id>] -- remove + persist a filter entry
bool CommManager::handleCmd(const char* cmd) {
    if (strcmp(cmd, "RELOAD_FILTERS") == 0) {
        filterManager_.reload();
        return true;
    }

    constexpr char kOverridePrefix[] = "SET_FILTER_OVERRIDE:";
    if (strncmp(cmd, kOverridePrefix, sizeof(kOverridePrefix) - 1) == 0) {
        const char* value = cmd + sizeof(kOverridePrefix) - 1;
        if (strcmp(value, "clear") == 0) {
            filterManager_.clearOverride();
            return true;
        }
        FilterManager::Mode mode;
        if (!FilterManager::parseModeName(value, mode)) {
            Serial.printf("[CommManager] unknown filter override mode '%s'\n", value);
            return false;
        }
        filterManager_.setOverride(mode);
        return true;
    }

    constexpr char kAddPrefix[] = "ADD_FILTER:";
    if (strncmp(cmd, kAddPrefix, sizeof(kAddPrefix) - 1) == 0) {
        return filterManager_.addFilterFromString(cmd + sizeof(kAddPrefix) - 1);
    }

    constexpr char kRemovePrefix[] = "REMOVE_FILTER:";
    if (strncmp(cmd, kRemovePrefix, sizeof(kRemovePrefix) - 1) == 0) {
        return filterManager_.removeFilterFromString(cmd + sizeof(kRemovePrefix) - 1);
    }

    Serial.printf("[CommManager] unknown CMD: %s\n", cmd);
    return false;
}

void CommManager::sendConfigData() {
    char buf[PACKET_MAX_PAYLOAD];
    const size_t n = config_.toJson(buf, sizeof(buf));
    if (n == 0) {
        Serial.println("[CommManager] config JSON doesn't fit in one packet -- see DOC/open-risks.md #2");
        sendNack();
        return;
    }

    Packet pkt{};
    pkt.header.type = static_cast<uint8_t>(MsgType::CONFIG_DATA);
    pkt.header.length = static_cast<uint16_t>(n);
    memcpy(pkt.payload, buf, n);
    client_->sendPacket(pkt);
    Serial.printf("[CommManager] sent CONFIG_DATA (%u bytes)\n", static_cast<unsigned>(n));
}

void CommManager::sendAck() {
    Packet pkt{};
    pkt.header.type = static_cast<uint8_t>(MsgType::ACK);
    pkt.header.length = 0;
    client_->sendPacket(pkt);
}

void CommManager::sendNack() {
    Packet pkt{};
    pkt.header.type = static_cast<uint8_t>(MsgType::NACK);
    pkt.header.length = 0;
    client_->sendPacket(pkt);
}

void CommManager::streamCanFrame(const CanFrame& frame) {
    if (!client_) {
        return;
    }
    // TODO(Stage 2): batch/backpressure handling instead of one send per frame.
    Packet pkt{};
    pkt.header.type = static_cast<uint8_t>(MsgType::CAN_FRAME);
    pkt.header.length = sizeof(CanFrame);
    memcpy(pkt.payload, &frame, sizeof(CanFrame));
    client_->sendPacket(pkt);
}
