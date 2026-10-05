"""Firmware-specific bootstrap metadata; common ROS behavior stays in launch."""
from dataclasses import dataclass
from types import MappingProxyType
from typing import Optional


@dataclass(frozen=True)
class BootstrapCapabilities:
    mavros_bootstrap: bool
    heartbeat_autodetection: bool


@dataclass(frozen=True)
class FirmwareProvider:
    name: str
    mavros_profile: Optional[str]
    capabilities: BootstrapCapabilities

    def require_bootstrap(self):
        if not self.capabilities.mavros_bootstrap:
            raise NotImplementedError(f"MAVROS_FIRMWARE={self.name}: not implemented")
        return self

    @property
    def parameter_files(self):
        self.require_bootstrap()
        return (
            f"{self.mavros_profile}_pluginlists.yaml",
            f"{self.mavros_profile}_config.yaml",
        )


_PROVIDERS = MappingProxyType({
    "ardupilot": FirmwareProvider("ardupilot", "apm", BootstrapCapabilities(True, True)),
    "px4": FirmwareProvider("px4", "px4", BootstrapCapabilities(True, True)),
    "betaflight": FirmwareProvider("betaflight", None, BootstrapCapabilities(False, False)),
    "inav": FirmwareProvider("inav", None, BootstrapCapabilities(False, False)),
    "mowgli": FirmwareProvider("mowgli", None, BootstrapCapabilities(False, False)),
})
PUBLIC_FIRMWARES = tuple(_PROVIDERS) + ("auto",)


def get_firmware_provider(firmware):
    """Return metadata for a concrete firmware; auto must be resolved first."""
    name = firmware.lower()
    if name not in _PROVIDERS:
        raise RuntimeError("MAVROS_FIRMWARE must be " + ", ".join(PUBLIC_FIRMWARES))
    return _PROVIDERS[name]
