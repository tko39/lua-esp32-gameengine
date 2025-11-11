#include "flags.h"
#if ENABLE_BLE
#include <Arduino.h>
#include <NimBLEDevice.h>
#include "simpleBle.hpp"

// Scan callbacks
void SimpleScanCallbacks::onResult(const NimBLEAdvertisedDevice *advertisedDevice)
{
    Serial.printf("Advertised Device found: %s\n", advertisedDevice->toString().c_str());
    std::string name = advertisedDevice->getName();
    if (!name.empty() && name.rfind("VR BOX", 0) == 0)
    { // starts with "VR BOX"
        Serial.printf("Found device starting with 'VR BOX': %s\n", name.c_str());
        NimBLEDevice::getScan()->stop();
        parentInstance->setAdvDevice(advertisedDevice);
        parentInstance->setDoConnect(true);
        parentInstance->setIsScanning(false);
    }
}

/** Callback to process the results of the completed scan. No restart. */
void SimpleScanCallbacks::onScanEnd(const NimBLEScanResults &results, int reason)
{
    Serial.printf("Scan Ended, reason: %d, device count: %d\n", reason, results.getCount());
    parentInstance->setIsScanning(false);
}

// Simple BLE
SimpleBle *SimpleBle::instance = nullptr;

/** Define a class to handle the callbacks when scan events are received */

int SimpleBle::notificationGapHandler(struct ble_gap_event *event, void *arg)
{
    if (instance)
    {
        return instance->notificationGapHandlerInstance(event);
    }
    return 0;
}

int SimpleBle::notificationGapHandlerInstance(struct ble_gap_event *event)
{
    // Check for the specific event you want to change
    if (event->type == BLE_GAP_EVENT_NOTIFY_RX)
    {
        // Debugging information:

        // char buf_conn[32];
        // sprintf(buf_conn, "0x%02X%02X (%hu)", event->notify_rx.conn_handle & 0xFF, (event->notify_rx.conn_handle >> 8) & 0xFF, event->notify_rx.conn_handle);

        // char buf_attr[32];
        // sprintf(buf_attr, "0x%02X%02X (%hu)", event->notify_rx.attr_handle & 0xFF, (event->notify_rx.attr_handle >> 8) & 0xFF, event->notify_rx.attr_handle);

        // std::string str = "TK Notification";
        // str += " from ";
        // str += buf_conn;
        // str += ", Attribute Handle= ";
        // str += buf_attr;
        // str += ", Data Hex= ";
        // for (size_t i = 0; i < OS_MBUF_PKTLEN(event->notify_rx.om); i++)
        // {
        //     char buf[4];
        //     sprintf(buf, "%02X ", event->notify_rx.om->om_data[i]);
        //     str += buf;
        // }
        // Serial.printf("%s\n", str.c_str());

        if (event->notify_rx.conn_handle == 0 && event->notify_rx.attr_handle == 23 && OS_MBUF_PKTLEN(event->notify_rx.om) >= 1)
        {
            uint8_t keyCode = event->notify_rx.om->om_data[0];
            switch (keyCode)
            {
            case 0x01:
                currentKeyDown = KEY_UP;
                break;
            case 0x02:
                currentKeyDown = KEY_DOWN;
                break;
            default:
                currentKeyDown = KEY_NONE;
                break;
            }
        }
        else
        {
            currentKeyDown = KEY_NONE;
        }
    }
    return 0; // Return 0 to allow default handling to proceed
}

/** Handles the provisioning of clients and connects / interfaces with the server */
bool SimpleBle::connectToServer()
{
    NimBLEClient *pClient = nullptr;

    if (NimBLEDevice::getCreatedClientCount())
    {
        pClient = NimBLEDevice::getClientByPeerAddress(advDevice->getAddress());
        if (pClient)
        {
            if (!pClient->connect(advDevice, false))
            {
                Serial.printf("Reconnect failed\n");
                return false;
            }
            Serial.printf("Reconnected client\n");
        }
        else
        {
            pClient = NimBLEDevice::getDisconnectedClient();
        }
    }

    if (!pClient)
    {
        if (NimBLEDevice::getCreatedClientCount() >= MYNEWT_VAL(BLE_MAX_CONNECTIONS))
        {
            Serial.printf("Max clients reached - no more connections available\n");
            return false;
        }

        pClient = NimBLEDevice::createClient();

        Serial.printf("New client created\n");

        pClient->setConnectionParams(12, 12, 0, 3000);

        if (!pClient->connect(advDevice))
        {
            NimBLEDevice::deleteClient(pClient);
            Serial.printf("Failed to connect, deleted client\n");
            return false;
        }
    }

    if (!pClient->isConnected())
    {
        if (!pClient->connect(advDevice))
        {
            Serial.printf("Failed to connect\n");
            return false;
        }
    }

    if (!pClient->secureConnection())
    {
        Serial.printf("Failed to secure connection\n");
        return false;
    }

    Serial.printf("Connected to: %s RSSI: %d\n", pClient->getPeerAddress().toString().c_str(), pClient->getRssi());
    return true;
}

bool SimpleBle::begin()
{
    /** Initialize NimBLE and set the device name */
    NimBLEDevice::init("NimBLE-Client");

    if (NimBLEDevice::setCustomGapHandler(notificationGapHandler))
    {
        Serial.println("Custom GAP handler set successfully, preserving original functionality.");
    }
    else
    {
        Serial.println("Failed to set custom GAP handler");
    }

    NimBLEDevice::setPower(3); /** 3dbm */
    NimBLEScan *pScan = NimBLEDevice::getScan();
    NimBLEDevice::setSecurityAuth(false, false, false);

    /** Set the callbacks to call when scan events occur, no duplicates */
    pScan->setScanCallbacks(&this->scanCallbacks, false);

    /** Set scan interval (how often) and window (how long) in milliseconds */
    pScan->setInterval(100);
    pScan->setWindow(100);
    pScan->setActiveScan(true);

    /** Start scanning for advertisers (once) */
    pScan->start(scanTimeMs);
    Serial.printf("Scanning once for device starting with 'VR BOX'...\n");
    Serial.printf("int size: %zu, float size: %zu\n", sizeof(int), sizeof(float));
    setIsScanning(true);

    while (isScanning)
    {
        delay(100);
    }

    if (doConnect)
    {
        doConnect = false;
        if (connectToServer())
        {
            Serial.printf("Success! Connected to VR BOX device.\n");
            return true;
        }
        else
        {
            Serial.printf("Failed to connect to VR BOX device.\n");
        }
    }
    else
    {
        Serial.printf("Did not find VR BOX, not connecting.\n");
    }

    return false;
}

SimpleBle::SimpleBle() : scanCallbacks(this), advDevice(nullptr), isScanning(false), doConnect(false)
{
    instance = this;
}

void SimpleBle::setIsScanning(bool scanning)
{
    isScanning = scanning;
}

void SimpleBle::setDoConnect(bool connect)
{
    doConnect = connect;
}

void SimpleBle::setAdvDevice(const NimBLEAdvertisedDevice *device)
{
    advDevice = device;
}

bool SimpleBle::isKeyDown(KeyCode key)
{
    if (instance)
    {
        return (instance->currentKeyDown == key);
    }
    // Return a default value or implement the actual callback logic as needed
    return false;
}
#endif // ENABLE_BLE