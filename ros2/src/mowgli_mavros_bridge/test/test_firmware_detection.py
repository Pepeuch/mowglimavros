#!/usr/bin/env python3
"""Probe actual libmavconn parsing over loopback UDP without an FCU."""
import socket
import struct
import subprocess
import sys
import threading
import unittest


EXECUTABLE = sys.argv.pop(1)


def heartbeat(autopilot, system=1, component=1, corrupt=False, version=1):
    # MAVLink v1 HEARTBEAT, with its standard CRC_EXTRA (50).
    payload = struct.pack("<IBBBBB", 0, 10, autopilot, 0, 0, 3)
    if version == 1:
        magic = b"\xfe"
        body = bytes((len(payload), 0, system, component, 0)) + payload
    else:
        magic = b"\xfd"
        body = bytes((len(payload), 0, 0, 0, system, component, 0, 0, 0)) + payload
    crc = 0xffff
    for value in body + bytes((50,)):
        tmp = value ^ (crc & 0xff)
        tmp = (tmp ^ (tmp << 4)) & 0xff
        crc = (crc >> 8) ^ (tmp << 8) ^ (tmp << 3) ^ (tmp >> 4)
    return magic + body + struct.pack("<H", crc ^ int(corrupt))


class FirmwareDetectionTest(unittest.TestCase):
    def probe(self, frames=(), system=1, component=1):
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sender:
            sender.bind(("127.0.0.1", 0))
            with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as reservation:
                reservation.bind(("127.0.0.1", 0))
                port = reservation.getsockname()[1]
            stop = threading.Event()
            received = []
            sender.settimeout(0.05)

            def send_frames():
                while not stop.wait(0.02):
                    for frame in frames:
                        sender.sendto(frame, ("127.0.0.1", port))
                    try:
                        received.append(sender.recvfrom(65535)[0])
                    except socket.timeout:
                        pass

            worker = threading.Thread(target=send_frames)
            worker.start()
            try:
                result = subprocess.run(
                    [EXECUTABLE, f"udp://127.0.0.1:{port}@127.0.0.1:{sender.getsockname()[1]}",
                     str(system), str(component), "400"],
                    capture_output=True, text=True, timeout=3)
            finally:
                stop.set()
                worker.join()
            self.assertEqual(received, [], "detection must not transmit MAVLink")
            # The probe releases the listener before MAVROS takes over.
            with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as listener:
                listener.bind(("127.0.0.1", port))
            return result

    def test_ardupilot_and_px4(self):
        for autopilot, firmware in ((3, "ardupilot"), (12, "px4")):
            for version in (1, 2):
                with self.subTest(firmware=firmware, version=version):
                    result = self.probe([heartbeat(autopilot, version=version)])
                    self.assertEqual(result.returncode, 0, result.stderr)
                    self.assertEqual(result.stdout.strip(), firmware)

    def test_only_configured_target_is_used(self):
        result = self.probe([heartbeat(3), heartbeat(12, system=42, component=7)], 42, 7)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stdout.strip(), "px4")

    def test_invalid_crc_peripheral_and_wrong_target_time_out(self):
        for frames in ([], [heartbeat(3, corrupt=True)], [heartbeat(8)],
                       [heartbeat(3, system=2)], [heartbeat(3, component=2)]):
            with self.subTest(frames=frames):
                result = self.probe(frames)
                self.assertNotEqual(result.returncode, 0)
                self.assertIn("no target FCU heartbeat", result.stderr)
                self.assertEqual(result.stdout, "")

    def test_unknown_autopilot_is_rejected(self):
        result = self.probe([heartbeat(0)])
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("cannot be identified", result.stderr)

    def test_invalid_transport_and_target_are_rejected(self):
        for args in (("invalid://", "1", "1"), ("udp://:0", "0", "1"),
                     ("udp://:0", "1", "256"), ("udp://:0", "1x", "1")):
            with self.subTest(args=args):
                result = subprocess.run([EXECUTABLE, *args], capture_output=True, text=True, timeout=3)
                self.assertNotEqual(result.returncode, 0)
                self.assertEqual(result.stdout, "")


if __name__ == "__main__":
    unittest.main()
