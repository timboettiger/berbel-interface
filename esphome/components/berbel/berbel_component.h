#pragma once

#include "esphome/core/component.h"
#include "esphome/core/preferences.h"
#include "esphome/components/web_server_base/web_server_base.h"
#include <NimBLEDevice.h>
#include <NimBLEScan.h>
#include <NimBLEClient.h>
#include <NimBLEAdvertisedDevice.h>
#include <string>
#include <vector>

namespace esphome {
namespace berbel {

static const char *CHAR_STATE_UUID = "F006F005-5745-4053-8043-62657262656C";
static const char *CHAR_BRIGHTNESS_UUID = "F006F006-5745-4053-8043-62657262656C";
static const char *CHAR_COLOR_UUID = "F006F007-5745-4053-8043-62657262656C";

static const uint8_t STATUS_FAN_L1_BYTE = 0;
static const uint8_t STATUS_FAN_L23_BYTE = 1;
static const uint8_t STATUS_LIGHT_TOP_BYTE = 2;
static const uint8_t STATUS_LIGHT_BOTTOM_BYTE = 4;
static const uint8_t STATUS_POSTRUN_BYTE = 5;
static const uint8_t BRIGHTNESS_BOTTOM_BYTE = 4;
static const uint8_t BRIGHTNESS_TOP_BYTE = 5;
static const uint8_t COLOR_BOTTOM_BYTE = 6;
static const uint8_t COLOR_TOP_BYTE = 7;

static const uint8_t LIGHT_ON_MASK = 0x10;
static const uint8_t POSTRUN_MASK = 0x90;
static const uint8_t FAN_L1_VALUE = 0x10;
static const uint8_t FAN_L2_VALUE = 0x01;
static const uint8_t FAN_L3_VALUE = 0x10;

static const uint16_t KELVIN_MIN = 2700;
static const uint16_t KELVIN_MAX = 6500;

static const uint32_t UPDATE_INTERVAL_MS = 18000;
static const uint32_t CONNECT_RETRY_MS = 5000;
static const uint32_t SCAN_DURATION_S = 8;

struct BerbelState {
  uint8_t fan_level{0};
  bool fan_postrun{false};
  bool light_top_on{false};
  bool light_bottom_on{false};
  uint8_t light_top_brightness{0};
  uint8_t light_bottom_brightness{0};
  uint8_t light_top_color{0};
  uint8_t light_bottom_color{0};
  bool connected{false};
  bool ble_ready{false};

  uint16_t light_top_color_kelvin() const {
    return KELVIN_MAX - (static_cast<uint32_t>(light_top_color) * (KELVIN_MAX - KELVIN_MIN) / 100);
  }
  uint16_t light_bottom_color_kelvin() const {
    return KELVIN_MAX - (static_cast<uint32_t>(light_bottom_color) * (KELVIN_MAX - KELVIN_MIN) / 100);
  }
};

struct ScannedDevice {
  std::string address;
  std::string name;
  int rssi;
};

enum class BerbelBleState {
  IDLE,
  SCANNING,
  CONNECTING,
  CONNECTED,
  DISCONNECTING,
};

class BerbelScanCallbacks;
class BerbelClientCallbacks;

class BerbelComponent : public Component {
 public:
  void setup() override;
  void loop() override;
  float get_setup_priority() const override;

  void set_update_interval(uint32_t ms);
  void set_device_mac(const std::string &mac);

  void start_scan();
  void stop_scan();
  void connect_to_device(const std::string &mac);
  void disconnect_device();

  void set_fan_level(uint8_t level);
  void set_light_top_brightness(uint8_t pct);
  void set_light_bottom_brightness(uint8_t pct);
  void set_light_top_on(bool on);
  void set_light_bottom_on(bool on);
  void set_light_top_color_kelvin(uint16_t kelvin);
  void set_light_bottom_color_kelvin(uint16_t kelvin);

  const BerbelState &get_state() const;
  const std::vector<ScannedDevice> &get_scanned_devices() const;
  bool is_scanning() const;
  BerbelBleState get_ble_state() const;

  void on_scan_result(NimBLEAdvertisedDevice *device);
  void on_scan_complete(NimBLEScanResults results);
  void on_ble_connect(NimBLEClient *client);
  void on_ble_disconnect(NimBLEClient *client, int reason);

 protected:
  void setup_http();
  void setup_ble();
  bool connect_internal(const std::string &mac);
  void disconnect_internal();
  bool read_state();
  bool execute_command(const uint8_t *cmd, size_t len);
  bool find_characteristics();
  void parse_state(const std::vector<uint8_t> &status, const std::vector<uint8_t> &brightness,
                   const std::vector<uint8_t> &colors);
  void save_mac(const std::string &mac);
  std::string load_mac();
  std::string state_to_json();
  std::string scanned_to_json();

  uint8_t build_brightness_command(uint8_t top_pct, uint8_t bottom_pct, uint8_t cmd_buf[31]);
  static uint8_t kelvin_to_pct(uint16_t kelvin);
  static uint16_t pct_to_kelvin(uint8_t pct);

  BerbelState state_;
  std::vector<ScannedDevice> scanned_devices_;
  BerbelBleState ble_state_{BerbelBleState::IDLE};
  std::string device_mac_;
  uint32_t update_interval_ms_{UPDATE_INTERVAL_MS};
  uint32_t last_update_ms_{0};
  uint32_t last_connect_attempt_ms_{0};

  NimBLEClient *ble_client_{nullptr};
  NimBLERemoteCharacteristic *char_state_{nullptr};
  NimBLERemoteCharacteristic *char_brightness_{nullptr};
  NimBLERemoteCharacteristic *char_color_{nullptr};

  ESPPreferenceObject pref_mac_;

  bool pending_connect_{false};
  std::string pending_mac_;
  bool pending_disconnect_{false};
};

class BerbelScanCallbacks : public NimBLEAdvertisedDeviceCallbacks {
 public:
  explicit BerbelScanCallbacks(BerbelComponent *component) : component_(component) {}
  void onResult(NimBLEAdvertisedDevice *device) override;

 private:
  BerbelComponent *component_;
};

class BerbelClientCallbacks : public NimBLEClientCallbacks {
 public:
  explicit BerbelClientCallbacks(BerbelComponent *component) : component_(component) {}
  void onConnect(NimBLEClient *client) override;
  void onDisconnect(NimBLEClient *client, int reason) override;

 private:
  BerbelComponent *component_;
};

}  // namespace berbel
}  // namespace esphome
