#pragma once

#include "settings.hpp"

namespace cmfc {
inline constexpr Tick sensor_fresh_ms = 350;
inline constexpr Tick tach_fresh_ms = 2500;
inline constexpr uint16_t ina_config = 0x0010;     // ADCRANGE = 1
inline constexpr uint16_t ina_adc_config = 0x3b42; // Trigger shunt/bus, 1052 us each, 16 samples.
inline constexpr uint8_t ina_address = 0x40;

enum class SampleError : uint8_t {
    None,
    Missing,
    Communication,
    WrongDevice,
    ConversionTimeout,
    Overflow,
    Trim,
    Short,
    Open,
    Invalid
};
const char *name(SampleError error);

struct Ina {
    Tick at{};
    SampleError error = SampleError::Missing;
    bool raw_valid{};
    bool diag_valid{};
    uint32_t bus_raw{}, shunt_raw{};
    uint16_t diag{};
};
struct Electrical {
    SampleError error = SampleError::Missing;
    float bus_v{}, shunt_v{}, current_a{}, power_w{};
};
Electrical convert(const Ina &sample, const Config &config, bool narrow = true);

struct Adc {
    Tick at{};
    bool present{};
    uint16_t count{};
};
struct Thermistor {
    SampleError error = SampleError::Missing;
    float ohm{}, celsius{};
};
Thermistor convert(const Adc &sample, const Config &config, unsigned fan);

struct Tach {
    Tick at{}, window_ms{}, last_pulse{};
    uint32_t pulses{};
    bool saw_pulse{};
    float rpm(unsigned ppr) const;
};
inline uint32_t pulse_delta(uint32_t total, uint32_t previous) { return total - previous; }

enum class Spindle : uint8_t { Unknown, Active, Standby };
enum class Activity : uint8_t { Unknown, No, Yes };
enum class Freshness : uint8_t { Missing, Fresh, Aging, Expired };
const char *name(Spindle state);
const char *name(Activity state);
const char *name(Freshness state);

struct Drive {
    std::array<char, 49> id{};
    bool temperature_valid{};
    float celsius{};
    uint32_t temperature_age_ms{};
    Spindle spindle{};
    Activity io{}, self_test{};
    uint32_t state_age_ms{};
};
bool valid(const Drive &drive);

struct Host {
    std::array<Drive, drive_capacity> drives{};
    unsigned count{};
    bool seen{}, connected{};
    Tick received_at{};
    bool update(const std::array<Drive, drive_capacity> &input, unsigned size, Tick now);
    Tick temperature_age(const Drive &drive, Tick now) const;
    Tick state_age(const Drive &drive, Tick now) const;
    Freshness freshness(const Drive &drive, Tick now, const Config &config) const;
    bool eligible(Tick now, const Config &config) const;
};

enum class PowerState : uint8_t { Unknown, Off, Starting, On };
const char *name(PowerState state);

struct Power {
    PowerState state = PowerState::Unknown;
    Tick on_since{};
    void update(Tick now, const Electrical &electrical, const Config &config);

  private:
    PowerState candidate = PowerState::Unknown;
    Tick candidate_since{};
};

enum Diagnostic : uint32_t {
    InaCommunication = 1u << 0,
    InaInvalid = 1u << 1,
    UpperNtc = 1u << 2,
    LowerNtc = 1u << 3,
    HostUnavailable = 1u << 4,
    HddHot = 1u << 5,
    PlenumHot = 1u << 6,
    UpperStall = 1u << 7,
    LowerStall = 1u << 8,
    PowerUnknown = 1u << 9,
    Uncalibrated = 1u << 10,
    ConfigInvalid = 1u << 11,
    WatchdogReset = 1u << 12,
    StorageFailure = 1u << 13,
    UsbError = 1u << 14,
    UpperUnconfirmed = 1u << 15,
    LowerUnconfirmed = 1u << 16
};
inline constexpr uint32_t protective_faults =
    InaCommunication | InaInvalid | UpperNtc | LowerNtc | HddHot | PlenumHot | UpperStall | LowerStall;

enum class Reason : uint8_t { Fixed, Uncalibrated, Protective, Startup, PowerUncertain, Policy };
const char *name(Reason reason);

struct Output {
    std::array<uint8_t, fan_count> pwm{100, 100};
    Reason reason = Reason::Uncalibrated;
};
struct Snapshot {
    Ina ina;
    std::array<Adc, fan_count> adc{};
    std::array<Tach, fan_count> tach{};
};
struct Evaluation {
    Electrical electrical;
    std::array<Thermistor, fan_count> thermistors{};
    uint32_t diagnostics = Uncalibrated;
    Output output;
};

// Future calibrated policy emits one equal duty and can decline to produce a demand.
struct Demand {
    bool calibrated{};
    uint8_t pwm{};
};
using CoolingPolicy = Demand (*)(const Evaluation &, const Host &, Tick);

struct Controller {
    Power power;
    Evaluation result;
    void evaluate(Tick now, const Settings &settings, const Snapshot &sensors, const Host &host,
                  uint32_t system_diagnostics, CoolingPolicy policy = nullptr);

  private:
    bool plenum_hot{}, protecting{}, recovering{}, hot_capacity_exceeded{};
    std::array<std::array<char, 49>, drive_capacity> hot_drives{};
    Tick recovery_since{};
    std::array<Tick, fan_count> rotation_check_since{};
    std::array<bool, fan_count> checking_rotation{};
    std::array<bool, fan_count> stalled{};
};
} // namespace cmfc
