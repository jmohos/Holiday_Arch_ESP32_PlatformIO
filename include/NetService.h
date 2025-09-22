#pragma once

#include <Arduino.h>
#include <Logging.h>
#include <WiFi.h>
#include <WiFiUdp.h>

#include "esp_wifi.h"

class NetService
{
public:
  // C-style callback (fast, no heap). Runs in the NetService task context (use core 0).
  // Keep it short; if you need heavy work, defer to another task/queue.
  typedef void (*PacketCallback)(const uint8_t *data, size_t len,
                                 const IPAddress &from, void *user);

  NetService(const String &ssid,
             const String &pass,
             IPAddress mcastAddr = IPAddress(239, 255, 0, 1),
             uint16_t port = 49400,
             uint8_t ttl = 1)
      : ssid_(ssid),
        pass_(pass),
        mcast_(mcastAddr),
        port_(port),
        ttl_(ttl)
  {
  }

  // Register the receive callback (optional).
  void onPacket(PacketCallback cb, void *user = nullptr)
  {
    cb_ = cb;
    user_ = user;
  }

    // FreeRTOS task entry point (pass &NetService instance as the pv parameter).
  static void task(void *pv)
  {
    reinterpret_cast<NetService *>(pv)->run();
  }


  int send(const uint8_t *data, size_t len)
  {
    if (!WiFi.isConnected())
      return -1;
    if (!udpTx_.beginPacket(mcast_, port_))
      return -2; // <-- multicast OK via beginPacket
    int w = udpTx_.write(data, len);
    udpTx_.endPacket();
    return w;
  }

  // Accessor functions for console interface.
  bool        isConnected() const { return WiFi.isConnected(); }
  bool        isListening() const { return listening_; }
  uint32_t    lastRxMs() const { return lastRxMs_; }
  uint8_t     lastFirstByte() const { return lastFirstByte_; }
  String      localIP() const { return WiFi.localIP().toString(); }
  String      gatewayIP() const { return WiFi.gatewayIP().toString(); }
  String      subnetMask() const { return WiFi.subnetMask().toString(); }
  String      ssid() const { return WiFi.SSID(); }
  String      bssid() const { return WiFi.BSSIDstr(); }
  String      macaddr() const { return WiFi.macAddress(); }
  int8_t      rssi() const { return WiFi.RSSI(); }
  int32_t     channel() const { return WiFi.channel(); }
  String      mcastIP() const { return mcast_.toString(); }
  uint16_t    mcastPort() const { return port_; }


private:
  void run()
  {

    ESP_LOGI("NET", "Starting NetService run task...");

    // Wi-Fi setup
    WiFi.persistent(false);
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false); // reduce jitter for RT workloads
    connectWifiBlocking_();

    // Join multicast
    (void)startMulticast_();

    // Re-join multicast when IP is (re)acquired
    WiFi.onEvent([](arduino_event_t *e)
                 {
                   // nothing here; static lambda requires captures—so we poll instead
                 });

    // RX loop
    static constexpr size_t BUF_SZ = 1472;
    uint8_t buf[BUF_SZ];

    for (;;)
    {
      // If Wi-Fi got dropped, try to reconnect and re-join mcast.
      if (!WiFi.isConnected())
      {
        listening_ = false;
        reconnectLoop_();
        (void)startMulticast_();
      }
      else if (!listening_)
      {
        (void)startMulticast_();
      }

      int packetSize = udpRx_.parsePacket();
      if (packetSize > 0)
      {
        int len = udpRx_.read(buf, min((int)BUF_SZ, packetSize));
        if (len > 0)
        {
          lastFirstByte_ = buf[0];
          lastRxMs_ = millis();
          if (cb_)
            cb_(buf, (size_t)len, udpRx_.remoteIP(), user_);
        }
      }
      vTaskDelay(pdMS_TO_TICKS(1)); // yield to Wi-Fi/LwIP
    }
  }

  // Called once on startup; will loop until connected
  void connectWifiBlocking_()
  {
    ESP_LOGI("NET", "Connecting to '%s'...", ssid_.c_str());

    // Clean start
    WiFi.disconnect(true /*wifioff*/, true /*eraseNVS*/);
    vTaskDelay(pdMS_TO_TICKS(200));
    WiFi.persistent(false);
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);

    // Ensure we scan the channels your AP may use (US=1..11; EU=1..13).
    wifi_country_t ctry = {"US", 1, 11, WIFI_COUNTRY_POLICY_AUTO};
    // If your AP might use ch 12/13, use: { "EU", 1, 13, WIFI_COUNTRY_POLICY_AUTO };
    esp_wifi_set_country(&ctry);

    // Helpful event log
    WiFi.onEvent([](arduino_event_t *ev)
                 {
    if (ev->event_id == ARDUINO_EVENT_WIFI_STA_DISCONNECTED) {
      ESP_LOGW("NET", "disconnected; reason=%d", ev->event_info.wifi_sta_disconnected.reason);
    }
    if (ev->event_id == ARDUINO_EVENT_WIFI_STA_GOT_IP) {
      ESP_LOGI("NET", "got IP: %s", WiFi.localIP().toString().c_str());
    } });

    // Scan for the target SSID (include hidden=true so beacons off-net still show when possible)
    int n = WiFi.scanNetworks(/*async=*/false, /*hidden=*/true);
    int idx = -1;
    for (int i = 0; i < n; ++i)
    {
      if (WiFi.SSID(i) == ssid_)
      {
        idx = i;
        break;
      }
    }

    if (idx >= 0)
    {
      const uint8_t *bssid = WiFi.BSSID(idx);
      int ch = WiFi.channel(idx);
      int rssi = WiFi.RSSI(idx);
      ESP_LOGI("NET", "found SSID on ch=%d RSSI=%d, joining via BSSID=%02X:%02X:%02X:%02X:%02X:%02X\n",
           ch, rssi, bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
     
      WiFi.begin(ssid_.c_str(), pass_.c_str(), ch, bssid, true /*connect*/);
    }
    else
    {
      ESP_LOGW("NET", "SSID not found in scan; trying blind connect (hidden or 5 GHz?)");
      WiFi.begin(ssid_.c_str(), pass_.c_str());
    }

    // Wait for connect with periodic retry
    uint32_t t0 = millis();
    const uint32_t tryWindowMs = 15000;
    while (WiFi.status() != WL_CONNECTED)
    {
      vTaskDelay(pdMS_TO_TICKS(200));
      if (millis() - t0 > tryWindowMs)
      {
        ESP_LOGW("NET", "connect timeout; rescanning and retrying...");
        t0 = millis();
        WiFi.disconnect(false, false);
        vTaskDelay(pdMS_TO_TICKS(300));
        // Re-run the scan + BSSID selection
        int n2 = WiFi.scanNetworks(false, true);
        int j = -1;
        for (int i = 0; i < n2; ++i)
          if (WiFi.SSID(i) == ssid_)
          {
            j = i;
            break;
          }
        if (j >= 0)
        {
          const uint8_t *b = WiFi.BSSID(j);
          int ch = WiFi.channel(j);
          WiFi.begin(ssid_.c_str(), pass_.c_str(), ch, b, true);
        }
        else
        {
          WiFi.begin(ssid_.c_str(), pass_.c_str());
        }
      }
    }
    ESP_LOGI("NET", "Connected. IP: %s", WiFi.localIP().toString().c_str());
  }

  void reconnectLoop_()
  {
    uint32_t lastPrint = 0;
    while (!WiFi.isConnected())
    {
      if (millis() - lastPrint > 1000)
      {
        ESP_LOGI("NET", "Waiting for reconnect...");
        lastPrint = millis();
      }

      // Try a gentle reconnect every 2 seconds
      WiFi.reconnect();
      vTaskDelay(pdMS_TO_TICKS(2000));
    }
    ESP_LOGI("NET", "Reconnected. IP: %s", WiFi.localIP().toString().c_str());
  }

  bool startMulticast_()
  {
    udpRx_.stop();
    bool ok = udpRx_.beginMulticast(mcast_, port_); // <-- only (group, port) on ESP32
    listening_ = ok;
    if (ok)
    {
      ESP_LOGI("NET", "Started multicast, listening on: %s:%u", mcast_.toString().c_str(), port_);
    }
    else
    {
      ESP_LOGI("NET", "Failed to listen on multicast!");
    }
    return ok;
  }

private:
  String ssid_;
  String pass_;
  IPAddress mcast_;
  uint16_t port_;
  uint8_t ttl_;

  // Callback
  PacketCallback cb_ = nullptr;
  void *user_ = nullptr;

  // UDP sockets
  WiFiUDP udpRx_;
  WiFiUDP udpTx_;

  // State
  volatile bool listening_ = false;
  volatile uint32_t lastRxMs_ = 0;
  volatile uint8_t lastFirstByte_ = 0;
};
