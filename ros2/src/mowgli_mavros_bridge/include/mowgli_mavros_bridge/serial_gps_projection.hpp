#pragma once

#include <cmath>
#include <cstdint>
#include <optional>

namespace mowgli_mavros_bridge
{
struct SerialGpsRaw
{
  uint8_t fix_type;
  int32_t lat_e7;
  int32_t lon_e7;
  int32_t altitude_mm;
  uint16_t hdop_centi;
  uint16_t satellites_visible;
  uint32_t horizontal_accuracy_mm;
  uint32_t vertical_accuracy_mm;
};

struct SerialGpsFix
{
  double latitude_deg;
  double longitude_deg;
  double altitude_m;
  std::optional<double> hdop;
  std::optional<double> horizontal_accuracy_m;
  std::optional<double> vertical_accuracy_m;
  uint16_t satellites_visible;
};

inline std::optional<SerialGpsFix> project_serial_gps(const SerialGpsRaw& raw)
{
  if (raw.fix_type < 3)
  {
    return std::nullopt;
  }
  const double lat = static_cast<double>(raw.lat_e7) / 1e7;
  const double lon = static_cast<double>(raw.lon_e7) / 1e7;
  if (!std::isfinite(lat) || !std::isfinite(lon) || std::abs(lat) > 90.0 ||
      std::abs(lon) > 180.0)
  {
    return std::nullopt;
  }
  SerialGpsFix fix{lat, lon, static_cast<double>(raw.altitude_mm) / 1000.0,
                   std::nullopt, std::nullopt, std::nullopt,
                   raw.satellites_visible};
  if (raw.hdop_centi != UINT16_MAX && raw.hdop_centi != 0)
  {
    fix.hdop = static_cast<double>(raw.hdop_centi) / 100.0;
  }
  if (raw.horizontal_accuracy_mm != UINT32_MAX && raw.horizontal_accuracy_mm != 0)
  {
    fix.horizontal_accuracy_m = static_cast<double>(raw.horizontal_accuracy_mm) / 1000.0;
  }
  if (raw.vertical_accuracy_mm != UINT32_MAX && raw.vertical_accuracy_mm != 0)
  {
    fix.vertical_accuracy_m = static_cast<double>(raw.vertical_accuracy_mm) / 1000.0;
  }
  return fix;
}
}  // namespace mowgli_mavros_bridge
