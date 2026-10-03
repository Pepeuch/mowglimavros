#pragma once

#include <cstddef>
#include <optional>

namespace mavros_gnss_adapter
{
// MAVLink deserialization zero-fills missing extension bytes. Require the
// complete field on the wire; zero alone cannot prove a populated extension.
template<typename Raw>
std::optional<double> ellipsoid_altitude(size_t payload_length, size_t offset, const Raw & raw)
{
  if (payload_length < offset || payload_length - offset < sizeof(raw.alt_ellipsoid) ||
    raw.alt_ellipsoid == 0)
  {
    return std::nullopt;
  }
  return raw.alt_ellipsoid / 1000.0;
}
}  // namespace mavros_gnss_adapter
