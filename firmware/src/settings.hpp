#pragma once

#include <array>
#include <cstdint>

namespace cmfc {
using Tick = uint64_t; // Monotonic milliseconds since boot.
constexpr unsigned fan_count = 2;
constexpr unsigned drive_capacity = 6;
constexpr unsigned schema_version = 1;
constexpr unsigned protocol_version = 1;

// Schema order is also the flash representation. Change the schema version if it changes.
// Type, JSON key, factory default, inclusive minimum, inclusive maximum.
#define CMFC_FIELDS(X)                                                                                       \
    X(float, shunt_ohm, 0.001f, 0.0001f, 0.01f)                                                              \
    X(float, current_gain, 1, 0.5f, 2)                                                                       \
    X(float, current_offset_a, 0, -2, 2)                                                                     \
    X(float, voltage_gain, 1, 0.9f, 1.1f)                                                                    \
    X(float, voltage_offset_v, 0, -0.5f, 0.5f)                                                               \
    X(uint32_t, ina_calibrated, 0, 0, 1)                                                                     \
    X(float, upper_r0_ohm, 10000, 1000, 100000)                                                              \
    X(float, upper_t0_c, 25, 0, 50)                                                                          \
    X(float, upper_beta_k, 3380, 2000, 6000)                                                                 \
    X(float, upper_divider_ohm, 10000, 1000, 100000)                                                         \
    X(float, upper_offset_c, 0, -15, 15)                                                                     \
    X(float, upper_adc_gain, 1, 0.95f, 1.05f)                                                                \
    X(uint32_t, upper_ntc_calibrated, 0, 0, 1)                                                               \
    X(float, lower_r0_ohm, 10000, 1000, 100000)                                                              \
    X(float, lower_t0_c, 25, 0, 50)                                                                          \
    X(float, lower_beta_k, 3380, 2000, 6000)                                                                 \
    X(float, lower_divider_ohm, 10000, 1000, 100000)                                                         \
    X(float, lower_offset_c, 0, -15, 15)                                                                     \
    X(float, lower_adc_gain, 1, 0.95f, 1.05f)                                                                \
    X(uint32_t, lower_ntc_calibrated, 0, 0, 1)                                                               \
    X(uint32_t, upper_ppr, 2, 1, 8)                                                                          \
    X(uint32_t, lower_ppr, 2, 1, 8)                                                                          \
    X(uint32_t, power_detection_validated, 0, 0, 1)                                                          \
    X(float, absent_bus_v, 1, 0.1f, 5)                                                                       \
    X(float, present_bus_v, 9, 6, 15)                                                                        \
    X(float, off_current_a, 0.02f, 0, 2)                                                                     \
    X(float, on_current_a, 0.1f, 0.001f, 5)                                                                  \
    X(uint32_t, power_debounce_ms, 1000, 200, 30000)                                                         \
    X(uint32_t, startup_grace_ms, 5000, 1000, 60000)                                                         \
    X(uint32_t, startup_boost_ms, 0, 0, 60000)                                                               \
    X(uint32_t, auto_fallback_pwm, 100, 50, 100)                                                             \
    X(uint32_t, stall_min_pwm, 0, 0, 100)                                                                    \
    X(uint32_t, stall_confirm_ms, 3000, 2000, 30000)                                                         \
    X(float, hdd_target_c, 35, 15, 39)                                                                       \
    X(float, hdd_emergency_c, 40, 25, 60)                                                                    \
    X(uint32_t, plenum_limit_enabled, 0, 0, 1)                                                               \
    X(float, plenum_emergency_c, 0, 0, 100)                                                                  \
    X(float, recovery_hysteresis_c, 2, 0.5f, 10)                                                             \
    X(uint32_t, recovery_ms, 3000, 200, 60000)                                                               \
    X(uint32_t, host_expected_ms, 60000, 1000, 300000)                                                       \
    X(uint32_t, host_fresh_ms, 90000, 1000, 600000)                                                          \
    X(uint32_t, host_expire_ms, 240000, 2000, 1800000)

struct Config {
#define FIELD(type, name, value, low, high) type name = value;
    CMFC_FIELDS(FIELD)
#undef FIELD
    bool operator==(const Config &) const = default;
};
bool valid(const Config &config);

enum class Mode : uint8_t { Auto, Fixed };

struct Operating {
    Mode mode = Mode::Auto;
    std::array<uint8_t, fan_count> pwm{100, 100};
    bool operator==(const Operating &) const = default;
};

bool valid(const Operating &operating);

struct Settings {
    Config config{};
    Operating operating{};
    bool operator==(const Settings &) const = default;
};
} // namespace cmfc
