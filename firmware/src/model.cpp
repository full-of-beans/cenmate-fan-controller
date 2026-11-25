#include "model.hpp"

#include <algorithm>
#include <cmath>
#include <string_view>

namespace cmfc {
template <typename T> static bool in_range(T value, T low, T high) { return value >= low && value <= high; }

bool valid(const Config &c) {
#define CHECK(type, key, value, low, high)                                                                   \
    if (!in_range<type>(c.key, low, high)) return false;
    CMFC_FIELDS(CHECK)
#undef CHECK
    return c.off_current_a < c.on_current_a && c.host_expected_ms <= c.host_fresh_ms &&
           c.host_fresh_ms < c.host_expire_ms &&
           c.hdd_target_c <= c.hdd_emergency_c - c.recovery_hysteresis_c &&
           (!c.plenum_limit_enabled || c.plenum_emergency_c >= 15);
}

bool valid(const Operating &o) {
    return (o.mode == Mode::Auto || o.mode == Mode::Fixed) && o.pwm[0] <= 100 && o.pwm[1] <= 100;
}

const char *name(SampleError e) {
    constexpr const char *names[]{
        "OK",       "MISSING",    "COMMUNICATION",     "WRONG_DEVICE",      "CONVERSION_TIMEOUT",
        "OVERFLOW", "TRIM_ERROR", "SHORT_OR_LOW_RAIL", "OPEN_OR_HIGH_RAIL", "INVALID"};
    return names[static_cast<unsigned>(e)];
}

static int32_t decode20(uint32_t raw) {
    const auto value = static_cast<int32_t>((raw >> 4) & 0xfffff);
    return value >= 0x80000 ? value - 0x100000 : value;
}

Electrical convert(const Ina &sample, const Config &c, bool narrow) {
    if (sample.error != SampleError::None) return {sample.error};
    if (!sample.raw_valid || !sample.diag_valid) return {SampleError::Invalid};
    if (!(sample.diag & 1u)) return {SampleError::Trim};
    const auto bus = decode20(sample.bus_raw);
    const auto shunt = decode20(sample.shunt_raw);
    if ((sample.diag & 0x0260u) || shunt == -524288 || shunt == 524287 || bus == 524287)
        return {SampleError::Overflow};
    const float bus_v = bus * 0.0001953125f;
    if (bus_v < 0 || bus_v > 85) return {SampleError::Invalid};

    Electrical result{SampleError::None};
    result.bus_v = std::max(0.0f, bus_v * c.voltage_gain + c.voltage_offset_v);
    result.shunt_v = shunt * (narrow ? 0.000000078125f : 0.0000003125f);
    result.current_a = c.current_gain * result.shunt_v / c.shunt_ohm + c.current_offset_a;
    result.power_w = result.bus_v * result.current_a;
    return result;
}

Thermistor convert(const Adc &sample, const Config &c, unsigned fan) {
    if (!sample.present) return {};
    if (sample.count > 4095) return {SampleError::Invalid};
    if (sample.count <= 4) return {SampleError::Short};
    if (sample.count >= 4091) return {SampleError::Open};
    const float fraction = sample.count / 4095.0f * (fan ? c.lower_adc_gain : c.upper_adc_gain);
    if (fraction >= 1 || fraction <= 0) return {SampleError::Invalid};
    const float resistance = (fan ? c.lower_divider_ohm : c.upper_divider_ohm) * fraction / (1 - fraction);
    const float r0 = fan ? c.lower_r0_ohm : c.upper_r0_ohm;
    const float t0 = 273.15f + (fan ? c.lower_t0_c : c.upper_t0_c);
    const float beta = fan ? c.lower_beta_k : c.upper_beta_k;
    const float kelvin = 1 / (1 / t0 + std::log(resistance / r0) / beta);
    const float celsius = kelvin - 273.15f + (fan ? c.lower_offset_c : c.upper_offset_c);
    if (!std::isfinite(resistance) || !std::isfinite(celsius) || resistance <= 0 || celsius < -55 ||
        celsius > 125)
        return {SampleError::Invalid};
    return {SampleError::None, resistance, celsius};
}

float Tach::rpm(unsigned ppr) const {
    return window_ms && ppr ? pulses * 60000.0f / (static_cast<float>(window_ms) * ppr) : 0;
}

const char *name(Spindle state) {
    constexpr const char *names[]{"UNKNOWN", "ACTIVE", "STANDBY"};
    return names[static_cast<unsigned>(state)];
}
const char *name(Activity state) {
    constexpr const char *names[]{"UNKNOWN", "NO", "YES"};
    return names[static_cast<unsigned>(state)];
}
const char *name(Freshness state) {
    constexpr const char *names[]{"MISSING", "FRESH", "AGING", "EXPIRED"};
    return names[static_cast<unsigned>(state)];
}

bool valid(const Drive &d) {
    if (!d.id.front() || d.id.back()) return false;
    for (char ch : d.id) {
        if (!ch) break;
        constexpr std::string_view allowed =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789._:-";
        if (allowed.find(ch) == allowed.npos) return false;
    }
    return (!d.temperature_valid || (d.celsius >= -20 && d.celsius <= 100)) &&
           d.spindle <= Spindle::Standby && d.io <= Activity::Yes && d.self_test <= Activity::Yes;
}

bool Host::update(const std::array<Drive, drive_capacity> &input, unsigned size, Tick now) {
    if (size > drive_capacity) return false;
    for (unsigned i = 0; i < size; ++i) {
        if (!valid(input[i])) return false;
        for (unsigned j = 0; j < i; ++j)
            if (std::string_view(input[i].id.data()) == input[j].id.data()) return false;
    }
    drives = input;
    count = size;
    received_at = now;
    seen = true;
    return true;
}

Tick Host::temperature_age(const Drive &d, Tick now) const {
    return now - received_at + d.temperature_age_ms;
}
Tick Host::state_age(const Drive &d, Tick now) const { return now - received_at + d.state_age_ms; }

Freshness Host::freshness(const Drive &d, Tick now, const Config &c) const {
    if (!seen || !d.temperature_valid) return Freshness::Missing;
    const Tick age = temperature_age(d, now);
    if (age >= c.host_expire_ms || now - received_at >= c.host_expire_ms) return Freshness::Expired;
    if (age > c.host_fresh_ms || now - received_at > c.host_fresh_ms) return Freshness::Aging;
    return Freshness::Fresh;
}

bool Host::eligible(Tick now, const Config &c) const {
    if (!connected || !count) return false;
    for (unsigned i = 0; i < count; ++i)
        if (freshness(drives[i], now, c) != Freshness::Fresh) return false;
    return true;
}

const char *name(PowerState state) {
    constexpr const char *names[]{"UNKNOWN", "OFF", "STARTING", "ON"};
    return names[static_cast<unsigned>(state)];
}

void Power::update(Tick now, const Electrical &e, const Config &c) {
    auto evidence = PowerState::Unknown;
    if (c.power_detection_validated && e.error == SampleError::None) {
        const float current = std::abs(e.current_a);
        // Voltage absence can corroborate OFF. A present rail with low current is ambiguous
        // after reset; a validated falling current threshold provides ON hysteresis.
        if (e.bus_v <= c.absent_bus_v && current <= c.off_current_a)
            evidence = PowerState::Off;
        else if (e.bus_v >= c.present_bus_v && (e.current_a >= c.on_current_a ||
                                                (state == PowerState::On && e.current_a > c.off_current_a)))
            evidence = PowerState::On;
    }
    if (evidence != candidate) {
        candidate = evidence;
        candidate_since = now;
    }
    if (evidence == PowerState::Unknown)
        state = PowerState::Unknown;
    else if (now - candidate_since >= c.power_debounce_ms) {
        if (evidence == PowerState::On && state != PowerState::On) on_since = now;
        state = evidence;
    } else if (evidence == PowerState::On && state != PowerState::On)
        state = PowerState::Starting;
    else if (evidence == PowerState::Off && state != PowerState::Off)
        state = PowerState::Unknown;
}

const char *name(Reason reason) {
    constexpr const char *names[]{"FIXED",   "UNCALIBRATED",    "PROTECTIVE",
                                  "STARTUP", "POWER_UNCERTAIN", "POLICY"};
    return names[static_cast<unsigned>(reason)];
}

void Controller::evaluate(Tick now, const Settings &settings, const Snapshot &sensors, const Host &host,
                          uint32_t diagnostics, CoolingPolicy policy) {
    const auto &c = settings.config;
    result.electrical = convert(sensors.ina, c);
    if (now - sensors.ina.at > sensor_fresh_ms) result.electrical.error = SampleError::Missing;
    const auto error = result.electrical.error;
    if (error == SampleError::Communication || error == SampleError::WrongDevice)
        diagnostics |= InaCommunication;
    else if (error != SampleError::None)
        diagnostics |= InaInvalid;

    power.update(now, result.electrical, c);
    if (power.state == PowerState::Unknown) diagnostics |= PowerUnknown;
    if (!host.eligible(now, c)) diagnostics |= HostUnavailable;

    for (unsigned i = 0; i < host.count; ++i) {
        const auto &drive = host.drives[i];
        if (host.freshness(drive, now, c) != Freshness::Fresh) continue;
        const auto known = std::find(hot_drives.begin(), hot_drives.end(), drive.id);
        if (drive.celsius >= c.hdd_emergency_c && known == hot_drives.end()) {
            const auto empty = std::find_if(hot_drives.begin(), hot_drives.end(),
                                            [](const auto &id) { return !id.front(); });
            if (empty == hot_drives.end())
                hot_capacity_exceeded = true;
            else
                *empty = drive.id;
        } else if (known != hot_drives.end() &&
                   drive.celsius <= c.hdd_emergency_c - c.recovery_hysteresis_c) {
            *known = {};
        }
    }
    if (hot_capacity_exceeded ||
        std::any_of(hot_drives.begin(), hot_drives.end(), [](const auto &id) { return id.front(); }))
        diagnostics |= HddHot;

    bool new_plenum_alarm = false;
    bool both_plenum_cool = true;
    std::array<bool, fan_count> recent_pulse{};
    for (unsigned i = 0; i < fan_count; ++i) {
        auto &t = result.thermistors[i];
        t = convert(sensors.adc[i], c, i);
        if (now - sensors.adc[i].at > sensor_fresh_ms) t.error = SampleError::Missing;
        const bool good = t.error == SampleError::None;
        if (!good) diagnostics |= i ? LowerNtc : UpperNtc;
        new_plenum_alarm |= good && t.celsius >= c.plenum_emergency_c;
        both_plenum_cool &= good && t.celsius <= c.plenum_emergency_c - c.recovery_hysteresis_c;
        const auto &tach = sensors.tach[i];
        recent_pulse[i] =
            tach.saw_pulse && now - tach.at <= tach_fresh_ms && now - tach.last_pulse <= tach_fresh_ms;
    }
    plenum_hot = c.plenum_limit_enabled && (new_plenum_alarm || (plenum_hot && !both_plenum_cool));
    if (plenum_hot) diagnostics |= PlenumHot;

    const bool power_ready = power.state == PowerState::On && now - power.on_since >= c.startup_grace_ms;
    for (unsigned i = 0; i < fan_count; ++i) {
        const auto &tach = sensors.tach[i];
        const bool requesting_rotation =
            c.stall_min_pwm && result.output.pwm[i] >= c.stall_min_pwm &&
            (settings.operating.mode != Mode::Fixed || settings.operating.pwm[i] >= c.stall_min_pwm);
        const bool confirmable = power_ready && recent_pulse[1 - i] && !recent_pulse[i] && tach.window_ms &&
                                 now - tach.at <= tach_fresh_ms;
        if (requesting_rotation && confirmable) {
            if (!checking_rotation[i]) rotation_check_since[i] = now;
            checking_rotation[i] = true;
            stalled[i] = now - rotation_check_since[i] >= c.stall_confirm_ms;
        } else {
            stalled[i] = false;
            checking_rotation[i] = false;
        }
        if (stalled[i])
            diagnostics |= i ? LowerStall : UpperStall;
        else if (!recent_pulse[i] && result.output.pwm[i] &&
                 (settings.operating.mode != Mode::Fixed || settings.operating.pwm[i]))
            diagnostics |= i ? LowerUnconfirmed : UpperUnconfirmed;
    }

    if (diagnostics & protective_faults) {
        protecting = true;
        recovering = false;
    } else if (protecting) {
        if (!recovering) recovery_since = now;
        recovering = true;
        protecting = now - recovery_since < c.recovery_ms;
    }

    result.diagnostics = diagnostics;
    Demand demand{};
    if (policy && settings.operating.mode == Mode::Auto && power_ready && !protecting)
        demand = policy(result, host, now);
    if (!demand.calibrated) result.diagnostics |= Uncalibrated;

    if (settings.operating.mode == Mode::Fixed) {
        result.output = {settings.operating.pwm, Reason::Fixed};
        return;
    }
    if (protecting)
        result.output = {{100, 100}, Reason::Protective};
    else if (power.state == PowerState::Starting ||
             (power.state == PowerState::On && now - power.on_since < c.startup_boost_ms))
        result.output = {{100, 100}, Reason::Startup};
    else if (power.state == PowerState::Unknown || power.state == PowerState::Off)
        result.output = {{100, 100}, Reason::PowerUncertain};
    else if (demand.calibrated && demand.pwm <= 100)
        result.output = {{demand.pwm, demand.pwm}, Reason::Policy};
    else {
        const auto pwm = static_cast<uint8_t>(c.auto_fallback_pwm);
        result.output = {{pwm, pwm}, Reason::Uncalibrated};
    }
}
} // namespace cmfc
