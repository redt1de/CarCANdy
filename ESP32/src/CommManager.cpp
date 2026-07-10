#include "CommManager.h"
#include <Arduino.h>
#include <string.h>

using namespace CarCANdy;

CommManager::CommManager(const char* commMode) : commMode_(commMode) {}

void CommManager::begin() {
    bool useSpi = strcmp(commMode_, "SPI") == 0;
    client_ = useSpi ? static_cast<CommClient*>(&spiClient_) : static_cast<CommClient*>(&uartClient_);

    if (!client_->begin()) {
        Serial.println("[CommManager] selected client failed to begin()");
    } else {
        Serial.printf("[CommManager] comm client started (mode=%s)\n", commMode_);
    }

    // Force an immediate CONFIG_GET on the first poll() rather than waiting
    // out a full retry interval.
    lastConfigRequestMs_ = millis() - kConfigRetryIntervalMs;
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

    if (!configReady_ && millis() - lastConfigRequestMs_ >= kConfigRetryIntervalMs) {
        requestConfig();
    }
    if (configRefreshPending_ && millis() >= configRefreshAtMs_) {
        configRefreshPending_ = false;
        requestConfig();
    }
}

void CommManager::requestConfig() {
    Packet pkt{};
    pkt.header.type = static_cast<uint8_t>(MsgType::CONFIG_GET);
    pkt.header.length = 0;
    client_->sendPacket(pkt);
    lastConfigRequestMs_ = millis();
    Serial.println("[CommManager] sent CONFIG_GET, awaiting reply...");
}

void CommManager::handlePacket(const Packet& pkt) {
    switch (static_cast<MsgType>(pkt.header.type)) {
    case MsgType::PING: {
        Packet ack{};
        ack.header.type = static_cast<uint8_t>(MsgType::ACK);
        ack.header.length = 0;
        client_->sendPacket(ack);
        break;
    }

    case MsgType::ACK:
        break; // fire-and-forget requests don't need to do anything with a bare ACK

    case MsgType::NACK:
        Serial.println("[CommManager] Teensy NACKed the last request");
        break;

    case MsgType::CONFIG_DATA: {
        const size_t len = pkt.header.length > PACKET_MAX_PAYLOAD ? PACKET_MAX_PAYLOAD : pkt.header.length;
        memcpy(configJson_, pkt.payload, len);
        configJson_[len] = '\0';
        configJsonLen_ = len;
        configReady_ = true;
        Serial.println("[CommManager] config received from Teensy");
        break;
    }

    case MsgType::CAN_FRAME: {
        if (pkt.header.length != sizeof(CanFrame)) {
            break;
        }
        CanFrame frame;
        memcpy(&frame, pkt.payload, sizeof(CanFrame));
        if (canFrameCallback_) {
            canFrameCallback_(frame);
        }
        break;
    }

    case MsgType::LOG_LINE: {
        char line[128];
        const size_t len = pkt.header.length < sizeof(line) - 1 ? pkt.header.length : sizeof(line) - 1;
        memcpy(line, pkt.payload, len);
        line[len] = '\0';
        if (logLineCallback_) {
            logLineCallback_(line);
        }
        break;
    }

    case MsgType::STATUS:
        // TODO(Stage 3): surface Teensy health/status on the Status page.
        break;

    default:
        Serial.printf("[CommManager] unexpected inbound MsgType 0x%02X\n", pkt.header.type);
        break;
    }
}

bool CommManager::sendConfigSet(const uint8_t* json, size_t len) {
    if (!client_ || len > PACKET_MAX_PAYLOAD) {
        return false;
    }
    Packet pkt{};
    pkt.header.type = static_cast<uint8_t>(MsgType::CONFIG_SET);
    pkt.header.length = static_cast<uint16_t>(len);
    memcpy(pkt.payload, json, len);
    if (!client_->sendPacket(pkt)) {
        return false;
    }
    // The Teensy applies + saves synchronously and replies ACK/NACK, but
    // this pass doesn't correlate that reply back to the HTTP request that
    // triggered it (see header comment) -- instead, re-request CONFIG_GET
    // shortly after so cachedConfigJson() catches up for the next GET.
    configRefreshPending_ = true;
    configRefreshAtMs_ = millis() + kConfigRefreshDelayMs;
    return true;
}

bool CommManager::sendCanFrame(const CanFrame& frame) {
    if (!client_) {
        return false;
    }
    Packet pkt{};
    pkt.header.type = static_cast<uint8_t>(MsgType::CAN_FRAME);
    pkt.header.length = sizeof(CanFrame);
    memcpy(pkt.payload, &frame, sizeof(CanFrame));
    return client_->sendPacket(pkt);
}
