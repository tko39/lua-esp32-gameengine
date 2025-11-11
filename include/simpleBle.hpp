#pragma once
#ifndef SIMPLE_BLE_HPP
#define SIMPLE_BLE_HPP
#include <NimBLEDevice.h>
#include "controller.hpp"

class SimpleBle;

class SimpleScanCallbacks : public NimBLEScanCallbacks
{
public:
    SimpleScanCallbacks(SimpleBle *parent) : parentInstance(parent) {}
    void onResult(const NimBLEAdvertisedDevice *advertisedDevice) override;

    void onScanEnd(const NimBLEScanResults &results, int reason) override;

private:
    SimpleBle *parentInstance;
};

class SimpleBle
{
public:
    SimpleBle();
    bool begin();

    void setIsScanning(bool scanning);
    void setDoConnect(bool connect);
    void setAdvDevice(const NimBLEAdvertisedDevice *device);
    static bool isKeyDown(KeyCode key);

private:
    static int notificationGapHandler(struct ble_gap_event *event, void *arg);
    int notificationGapHandlerInstance(struct ble_gap_event *event);
    bool connectToServer();

    static SimpleBle *instance;
    SimpleScanCallbacks scanCallbacks;
    bool isScanning;
    bool doConnect;
    const NimBLEAdvertisedDevice *advDevice;
    static const uint32_t scanTimeMs = 2000;
    KeyCode currentKeyDown = KEY_NONE;
};

#endif // SIMPLE_BLE_HPP