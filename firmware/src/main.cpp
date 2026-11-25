#include "platform.hpp"
#include "protocol.hpp"

#include <algorithm>

#include "hardware/watchdog.h"
#include "pico/platform.h"
#include "pico/rand.h"
#include "tusb.h"

namespace {
cmfc::Runtime runtime(cmfc::pico::flash_storage());
cmfc::Protocol protocol(runtime);

void pump_usb(bool ready) {
    if (!ready) return;
    tud_task_ext(0, false);
    const bool connected = tud_cdc_connected() && !tud_suspended();
    const bool was_connected = runtime.host.connected;
    protocol.connection(connected);
    if (!connected) {
        if (was_connected) {
            tud_cdc_write_clear();
            tud_cdc_read_flush();
        }
        return;
    }

    const auto pending = protocol.pending();
    if (!pending.empty()) {
        const auto size = std::min<size_t>(pending.size(), tud_cdc_write_available());
        if (size) {
            protocol.consume(tud_cdc_write(pending.data(), static_cast<uint32_t>(size)));
            tud_cdc_write_flush();
        }
    }
    // Do not consume a request that cannot get a response. Endpoint backpressure bounds RX.
    for (unsigned bytes = 0; bytes < 128 && protocol.pending().empty() && tud_cdc_available(); ++bytes) {
        const int byte = tud_cdc_read_char();
        if (byte < 0) break;
        protocol.feed(static_cast<char>(byte), cmfc::pico::now_ms());
    }
    protocol.tick(cmfc::pico::now_ms(), !tud_cdc_available() && tud_cdc_write_available() >= 64);
}
} // namespace

int main() {
    runtime.watchdog_reset = watchdog_caused_reboot();
    watchdog_enable(2000, false);
    runtime.load();
    runtime.evaluate(cmfc::pico::now_ms());
    cmfc::pico::init_pwm(runtime.controller.result.output);
    cmfc::pico::start_acquisition(cmfc::pico::now_ms());
    runtime.boot_id = get_rand_64();
    const tusb_rhport_init_t usb{TUSB_ROLE_DEVICE, TUSB_SPEED_FULL};
    const bool usb_ready = tusb_init(0, &usb);

    cmfc::Tick last_control{};
    auto previous = runtime.controller.result.output.pwm;
    auto operating = runtime.settings.operating;
    auto revision = runtime.config_revision;
    while (true) {
        const auto now = cmfc::pico::now_ms();
        if (cmfc::pico::acquire(now, runtime.sensors) || now - last_control >= 100 ||
            operating != runtime.settings.operating || revision != runtime.config_revision) {
            operating = runtime.settings.operating;
            revision = runtime.config_revision;
            last_control = cmfc::pico::now_ms();
            runtime.evaluate(last_control);
            const auto &output = runtime.controller.result.output;
            if (previous != output.pwm) {
                cmfc::pico::apply_pwm(output);
                previous = output.pwm;
            }
            watchdog_update();
        }
        pump_usb(usb_ready);
        tight_loop_contents();
    }
}
