#!/usr/bin/env python3

from amaranth import *
from amaranth.lib.fifo import AsyncFIFOBuffered
from luna import top_level_cli
from luna.gateware.interface.ulpi import UTMITranslator
from luna.gateware.usb.usb2.device import USBDevice
from luna.gateware.usb.usb2.endpoints.stream import (
    USBStreamInEndpoint,
    USBStreamOutEndpoint,
)


class USBMouseProxy(Elaboratable):
    def elaborate(self, platform):
        m = Module()

        # --- Clock Domains ---
        m.domains.sync = ClockDomain()
        m.domains.usb = ClockDomain()

        # --- Platform Resources ---
        aux_phy = platform.request("aux_phy")  # To PC
        target_phy = platform.request("target_phy")  # To Mouse
        target_vbus_en = platform.request("target_c_vbus_en")

        # --- ULPI <-> UTMI Translators ---
        m.submodules.aux_utmi = aux_utmi = UTMITranslator(ulpi=aux_phy)
        m.submodules.target_utmi = target_utmi = UTMITranslator(ulpi=target_phy)

        # --- Power up the Mouse ---
        m.d.comb += target_vbus_en.o.eq(1)

        # --- USB Device Controller (PC facing) ---
        m.submodules.device = device = USBDevice(bus=aux_utmi)

        # --- OUT endpoint: PC -> Mouse ---
        out_ep = USBStreamOutEndpoint(
            endpoint_number=1,
            max_packet_size=8,
        )
        device.add_endpoint(out_ep)

        m.d.comb += [
            target_utmi.tx_data.eq(out_ep.stream.payload),
            target_utmi.tx_valid.eq(out_ep.stream.valid),
            out_ep.stream.ready.eq(target_utmi.tx_ready),
        ]

        # --- IN endpoint: Mouse -> PC ---
        in_ep = USBStreamInEndpoint(
            endpoint_number=1,
            max_packet_size=8,
        )
        device.add_endpoint(in_ep)

        m.d.comb += [
            in_ep.stream.valid.eq(target_utmi.rx_valid),
            in_ep.stream.payload.eq(target_utmi.rx_data),
            target_utmi.tx_ready.eq(in_ep.stream.ready),
        ]

        # --- Always connect ---
        m.d.comb += device.connect.eq(1)

        return m


if __name__ == "__main__":
    top_level_cli(USBMouseProxy)
