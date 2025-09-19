// SettingsStore.h
#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include <stdint.h>
#include <string.h>

namespace Persist {

class SettingsStore {
public:
  // ---- Tunables / format ids ----
  static constexpr uint32_t kMagic   = 0x53474631; // 'SGF1' or any tag you like
  static constexpr uint16_t kVersion = 0x0001;     // bump if layout changes

  // Limits (SSID max 32, WPA2 pass up to 63)
  static constexpr size_t kMaxSsid = 32;
  static constexpr size_t kMaxPwd  = 64;

  // On-flash blob (explicit size; CRC covers all fields except crc)
  struct __attribute__((packed)) Blob {
    uint32_t magic;           // kMagic
    uint16_t version;         // kVersion
    uint8_t  deviceId;        // 0..254
    uint8_t  _pad0;           // keep alignment stable
    char     ssid[kMaxSsid];  // null-terminated
    char     pwd[kMaxPwd];    // null-terminated
    uint8_t  volume;          // 0-30
    uint8_t  reserved[63];    // future use; must be zeroed
    uint32_t crc32;           // CRC32 of [magic..reserved]
  };

  SettingsStore(const char* nvsNamespace = "cfg",
                const char* blobKey = "blob")
  : ns_(nvsNamespace), key_(blobKey) {
    memset(&blob_, 0, sizeof(blob_));
  }

  // Initialize NVS, load blob, validate, or seed defaults+save.
  bool begin() {
    prefs_.begin(ns_.c_str(), /*readOnly=*/false);
    if (!load()) {
      // If we don't have a valid storage area, create one with defaults.
      setDefaults();
      save(); // persist defaults

      // Verify that seed was properly loaded
      if (!load()) {
        // Failed to allocate and seed persistent memory space!
        return false;
      }
    }
    return true;
  }

  // Reload from NVS (returns true if valid)
  bool load() {
    size_t need = prefs_.getBytesLength(key_.c_str());
    if (need != sizeof(Blob)) return false;
    Blob tmp{};
    size_t got = prefs_.getBytes(key_.c_str(), &tmp, sizeof(tmp));
    if (got != sizeof(tmp)) return false;
    if (!isValid(tmp)) return false;
    blob_ = tmp;
    return true;
  }

  // Save to NVS (recomputes CRC)
  bool save() {
    finalize(blob_);
    size_t w = prefs_.putBytes(key_.c_str(), &blob_, sizeof(blob_));
    return w == sizeof(blob_);
  }

  // Reset in-RAM values to sane defaults (not auto-saved)
  void setDefaults() {
    memset(&blob_, 0, sizeof(blob_));
    blob_.magic   = kMagic;
    blob_.version = kVersion;
    blob_.deviceId = 0x00;   // default node id

    // Preset the wifi SSID and password defaults
    copyCString(String("showiot"), blob_.ssid, sizeof(blob_.ssid));
    copyCString(String("IOTNetwork4me!"), blob_.pwd, sizeof(blob_.pwd));
    //blob_.ssid[0] = '\0';    // blank SSID
    //blob_.pwd[0]  = '\0';    // blank password

    // Preset the volume
    blob_.volume = 20;

    // reserved[] already zero
    blob_.crc32   = crc32Of(blob_); // keep consistent even before save
  }

  // ---- Accessors ----
  uint8_t deviceId() const { return blob_.deviceId; }
  String  ssid()     const { return String(blob_.ssid); }
  String  password() const { return String(blob_.pwd);  }
  uint8_t volume()   const { return blob_.volume; }

  void setDeviceId(uint8_t id) {
    if (id <= 0xFE) blob_.deviceId = id;   // 0..254
  }
  void setSsid(const String& s)  { copyCString(s, blob_.ssid, sizeof(blob_.ssid)); }
  void setPassword(const String& p) { copyCString(p, blob_.pwd, sizeof(blob_.pwd)); }

  void setVolume(uint8_t vol) {
    if (vol > 30) {
      blob_.volume = 30;
    } else {
      blob_.volume = vol; }
  }

  // Access underlying blob if needed
  const Blob& blob() const { return blob_; }

  // Validate external blob (magic/version/crc)
  static bool isValid(const Blob& b) {
    if (b.magic != kMagic) return false;
    if (b.version != kVersion) return false;
    uint32_t expect = crc32Of(b);
    return expect == b.crc32;
  }

private:
  // ---- Internals ----
  static void copyCString(const String& s, char* dst, size_t cap) {
    size_t n = min(s.length(), cap ? cap - 1 : 0);
    if (cap) {
      memcpy(dst, s.c_str(), n);
      dst[n] = '\0';
    }
  }

  static uint32_t crc32_update(uint32_t crc, const uint8_t* data, size_t len) {
    crc = ~crc;
    for (size_t i = 0; i < len; ++i) {
      uint8_t c = data[i];
      crc ^= c;
      for (int k = 0; k < 8; ++k)
        crc = (crc >> 1) ^ (0xEDB88320u & (-(int)(crc & 1)));
    }
    return ~crc;
  }

  static uint32_t crc32Of(const Blob& b) {
    uint32_t crc = 0;
    // Compute over everything up to (but not including) crc32
    crc = crc32_update(crc, reinterpret_cast<const uint8_t*>(&b), offsetof(Blob, crc32));
    return crc;
  }

  static void finalize(Blob& b) {
    // Ensure invariant fields + zero reserved before CRC
    b.magic = kMagic;
    b.version = kVersion;
    // Ensure strings are null-terminated
    b.ssid[sizeof(b.ssid) - 1] = '\0';
    b.pwd [sizeof(b.pwd)  - 1] = '\0';
    // (reserved is expected to be already zero)
    b.crc32 = crc32Of(b);
  }

private:
  Preferences prefs_;
  String ns_;
  String key_;
  Blob   blob_;
};

} // namespace Persist
