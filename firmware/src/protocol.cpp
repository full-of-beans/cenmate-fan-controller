#include "protocol.hpp"

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include <type_traits>

#include "build_info.hpp"
#include "yyjson.h"

namespace cmfc {
namespace {
using Value = yyjson_val;
constexpr Tick receive_timeout_ms = 2000;
constexpr auto yes(bool value) { return value ? "true" : "false"; }

std::string_view str(Value *v) {
    if (!yyjson_is_str(v)) return {};
    return {yyjson_get_str(v), yyjson_get_len(v)};
}

bool keys(Value *object, std::initializer_list<std::string_view> allowed) {
    if (!yyjson_is_obj(object) || yyjson_obj_size(object) > allowed.size()) return false;
    size_t index, count;
    Value *key, *value;
    yyjson_obj_foreach(object, index, count, key, value) {
        const auto s = str(key);
        if (std::find(allowed.begin(), allowed.end(), s) == allowed.end() ||
            yyjson_obj_getn(object, s.data(), s.size()) != value)
            return false;
    }
    return true;
}

bool integer(Value *value, uint32_t low, uint32_t high, uint32_t &output) {
    if (!yyjson_is_uint(value)) return false;
    const auto number = yyjson_get_uint(value);
    if (number < low || number > high) return false;
    output = static_cast<uint32_t>(number);
    return true;
}

bool boolean(Value *value, bool &output) {
    if (!yyjson_is_bool(value)) return false;
    output = yyjson_get_bool(value);
    return true;
}

template <typename T> bool number(Value *value, T low, T high, T &output) {
    if constexpr (std::is_same_v<T, uint32_t>)
        return integer(value, low, high, output);
    else {
        if (!yyjson_is_num(value)) return false;
        const double n = yyjson_get_num(value);
        if (!(n >= low && n <= high)) return false;
        output = static_cast<T>(n);
        return true;
    }
}

bool config_patch(Value *object, Config &config) {
    if (!yyjson_is_obj(object)) return false;
    size_t index, count;
    Value *key, *value;
    yyjson_obj_foreach(object, index, count, key, value) {
        const auto s = str(key);
        if (yyjson_obj_getn(object, s.data(), s.size()) != value) return false;
#define FIELD(type, field, default_value, low, high)                                                         \
    if (s == #field) {                                                                                       \
        if (!number<type>(value, low, high, config.field)) return false;                                     \
        continue;                                                                                            \
    }
        CMFC_FIELDS(FIELD)
#undef FIELD
        return false;
    }
    return valid(config);
}

bool parse_format(Value *value, Format &format) {
    if (!value || str(value) == "FULL")
        format = Format::Full;
    else if (str(value) == "RAW")
        format = Format::Raw;
    else if (str(value) == "PHYSICAL")
        format = Format::Physical;
    else
        return false;
    return true;
}

bool parse_pwm(Value *object, Operating &operating, bool required) {
    auto *upper = yyjson_obj_get(object, "upper_pwm");
    auto *lower = yyjson_obj_get(object, "lower_pwm");
    if (!upper && !lower && required) return false;
    for (unsigned fan = 0; fan < fan_count; ++fan) {
        auto *v = fan ? lower : upper;
        if (!v) continue;
        uint32_t pwm{};
        if (!integer(v, 0, 100, pwm)) return false;
        operating.pwm[fan] = static_cast<uint8_t>(pwm);
    }
    return true;
}

template <typename Enum> bool parse_state(Value *value, Enum &state) {
    if (!value || yyjson_is_null(value)) return true;
    for (unsigned i = 0; i < 3; ++i)
        if (str(value) == name(static_cast<Enum>(i))) {
            state = static_cast<Enum>(i);
            return true;
        }
    return false;
}

bool parse_drives(Value *array, Host &host, Tick now) {
    const auto count = yyjson_arr_size(array);
    if (!yyjson_is_arr(array) || count > drive_capacity) return false;
    std::array<Drive, drive_capacity> drives{};
    for (unsigned i = 0; i < count; ++i) {
        auto *item = yyjson_arr_get(array, i);
        if (!keys(item, {"drive_id", "temperature_c", "temperature_age_ms", "spindle", "io", "self_test",
                         "state_age_ms"}))
            return false;
        auto &d = drives[i];
        const auto id = str(yyjson_obj_get(item, "drive_id"));
        if (id.empty() || id.size() >= d.id.size() || id.find('\0') != id.npos) return false;
        std::copy(id.begin(), id.end(), d.id.begin());
        auto *temperature = yyjson_obj_get(item, "temperature_c");
        auto *temperature_age = yyjson_obj_get(item, "temperature_age_ms");
        d.temperature_valid = temperature && !yyjson_is_null(temperature);
        if (d.temperature_valid) {
            if (!number<float>(temperature, -20, 100, d.celsius) ||
                !integer(temperature_age, 0, UINT32_MAX, d.temperature_age_ms))
                return false;
        } else if (temperature_age && !yyjson_is_null(temperature_age))
            return false;
        if (!parse_state(yyjson_obj_get(item, "spindle"), d.spindle) ||
            !parse_state(yyjson_obj_get(item, "io"), d.io) ||
            !parse_state(yyjson_obj_get(item, "self_test"), d.self_test))
            return false;
        auto *state_age = yyjson_obj_get(item, "state_age_ms");
        if (d.spindle != Spindle::Unknown || d.io != Activity::Unknown || d.self_test != Activity::Unknown) {
            if (!integer(state_age, 0, UINT32_MAX, d.state_age_ms)) return false;
        } else if (state_age && !yyjson_is_null(state_age))
            return false;
    }
    return host.update(drives, static_cast<unsigned>(count), now);
}

struct DiagnosticEntry {
    uint32_t mask;
    const char *code;
    const char *severity;
};
constexpr DiagnosticEntry diagnostics[]{
    {InaCommunication, "INA_COMMUNICATION", "fault"},
    {InaInvalid, "INA_INVALID", "fault"},
    {UpperNtc, "UPPER_NTC", "fault"},
    {LowerNtc, "LOWER_NTC", "fault"},
    {HostUnavailable, "HOST_UNAVAILABLE", "info"},
    {HddHot, "HDD_OVER_TEMPERATURE", "fault"},
    {PlenumHot, "PLENUM_OVER_TEMPERATURE", "fault"},
    {UpperStall, "UPPER_STALL", "fault"},
    {LowerStall, "LOWER_STALL", "fault"},
    {PowerUnknown, "POWER_UNKNOWN", "warning"},
    {Uncalibrated, "AUTO_UNCALIBRATED", "info"},
    {ConfigInvalid, "CONFIG_INVALID", "warning"},
    {WatchdogReset, "WATCHDOG_RESET", "warning"},
    {StorageFailure, "STORAGE_FAILURE", "fault"},
    {UsbError, "USB_PROTOCOL_ERROR", "warning"},
    {UpperUnconfirmed, "UPPER_UNCONFIRMED", "warning"},
    {LowerUnconfirmed, "LOWER_UNCONFIRMED", "warning"},
};

void nullable(Protocol &p, bool available, double value) {
    if (available)
        p.output("%.9g", value);
    else
        p.output("null");
}

void time_or_null(Protocol &p, bool available, Tick time) {
    if (available)
        p.output("%llu", static_cast<unsigned long long>(time));
    else
        p.output("null");
}

void operating(Protocol &p, const Operating &settings) {
    p.output("{\"mode\":\"%s\",\"upper_pwm\":%u,\"lower_pwm\":%u}",
             settings.mode == Mode::Fixed ? "FIXED" : "AUTO", unsigned(settings.pwm[0]),
             unsigned(settings.pwm[1]));
}

void config_object(Protocol &protocol, const Config &config) {
    protocol.output("{\"schema_version\":%u", schema_version);
#define FIELD(type, key, value, low, high) protocol.output(",\"" #key "\":%.9g", double(config.key));
    CMFC_FIELDS(FIELD)
#undef FIELD
    protocol.output("}");
}
} // namespace

std::string_view Protocol::pending() const { return {tx.data() + sent, tx_size - sent}; }

void Protocol::consume(size_t count) {
    sent += std::min(count, tx_size - sent);
    if (sent == tx_size) begin();
}

void Protocol::begin() {
    tx_size = sent = 0;
    formatting_error = false;
}

void Protocol::output(const char *format, ...) {
    if (formatting_error) return;
    va_list args;
    va_start(args, format);
    const int size = std::vsnprintf(tx.data() + tx_size, tx.size() - tx_size, format, args);
    va_end(args);
    if (size < 0 || static_cast<size_t>(size) >= tx.size() - tx_size)
        formatting_error = true;
    else
        tx_size += size;
}

void Protocol::finish() {
    output("}\n");
    if (formatting_error) error(-1, "response_too_large");
}

void Protocol::error(int64_t id, const char *code, bool applied) {
    if (std::strcmp(code, "storage_failure") != 0) runtime.latched |= UsbError;
    begin();
    output("{\"id\":");
    time_or_null(*this, id >= 0, static_cast<Tick>(id));
    output(",\"ok\":false,\"error\":\"%s\",\"applied\":%s}\n", code, yes(applied));
}

void Protocol::connection(bool connected) {
    if (runtime.host.connected == connected) return;
    runtime.host.connected = connected;
    stream = false;
    begin();
    rx_size = 0;
    discarding = false;
}

bool Protocol::feed(char byte, Tick now) {
    if (!runtime.host.connected || !pending().empty()) return false;
    last_byte = now;
    if (byte == '\n') {
        if (discarding)
            error(-1, "message_too_large");
        else if (rx_size)
            dispatch(now);
        rx_size = 0;
        discarding = false;
    } else if (!discarding) {
        if (rx_size == receive_limit)
            discarding = true;
        else
            rx[rx_size++] = byte;
    }
    return true;
}

void Protocol::tick(Tick now, bool allow_stream) {
    if (!runtime.host.connected) return;
    if ((rx_size || discarding) && now - last_byte >= receive_timeout_ms && pending().empty()) {
        error(-1, discarding ? "message_too_large" : "partial_timeout");
        rx_size = 0;
        discarding = false;
    }
    if (!stream || now - last_stream < interval_ms) return;
    last_stream = now;
    if (!allow_stream || !pending().empty() || rx_size || discarding) {
        ++dropped;
        return;
    }
    output("{\"event\":\"telemetry\",\"protocol\":%u,\"version\":\"%s\",\"sequence\":%u,"
           "\"status\":",
           protocol_version, CMFC_VERSION, unsigned(++sequence));
    status(now);
    output(",\"sensors\":");
    sensors(now, stream_format);
    finish();
}

void Protocol::dispatch(Tick now) {
    yyjson_alc allocator{};
    if (!yyjson_alc_pool_init(&allocator, arena.data(), arena.size())) {
        error(-1, "parser_unavailable");
        return;
    }
    // In-situ decoding uses padded RX storage and a fixed allocator. No parsed value survives dispatch.
    std::fill_n(rx.data() + rx_size, 8, '\0');
    auto *doc = yyjson_read_opts(rx.data(), rx_size, YYJSON_READ_INSITU, &allocator, nullptr);
    if (!doc) {
        error(-1, "malformed_json");
        return;
    }
    auto *root = yyjson_doc_get_root(doc);
    uint32_t identifier{};
    if (!integer(yyjson_obj_get(root, "id"), 0, UINT32_MAX, identifier)) {
        error(-1, "invalid_id");
        return;
    }
    const auto cmd = str(yyjson_obj_get(root, "cmd"));
    const auto get = [root](const char *key) { return yyjson_obj_get(root, key); };
    const auto invalid = [&] { error(identifier, "invalid_request"); };
    const auto stored = [&](bool ok) {
        if (!ok) error(identifier, "storage_failure", true);
        return ok;
    };
    const auto success = [&] { output("{\"id\":%u,\"ok\":true", unsigned(identifier)); };

    if (cmd == "set_mode" || cmd == "set_pwm") {
        const bool changing_mode = cmd == "set_mode";
        const bool valid_keys = changing_mode
                                    ? keys(root, {"id", "cmd", "mode", "upper_pwm", "lower_pwm", "persist"})
                                    : keys(root, {"id", "cmd", "upper_pwm", "lower_pwm", "persist"});
        auto operating_settings = runtime.settings.operating;
        bool persist = true;
        if (!valid_keys || (get("persist") && !boolean(get("persist"), persist)) ||
            !parse_pwm(root, operating_settings, !changing_mode))
            return invalid();
        if (changing_mode) {
            const auto mode = str(get("mode"));
            if (mode == "AUTO")
                operating_settings.mode = Mode::Auto;
            else if (mode == "FIXED")
                operating_settings.mode = Mode::Fixed;
            else
                return invalid();
        } else if (operating_settings.mode != Mode::Fixed) {
            return error(identifier, "requires_fixed");
        }
        if (!stored(runtime.set_operating(operating_settings, persist))) return;
        success();
        output(",\"requested\":");
        operating(*this, operating_settings);
        output(",\"persisted\":%s", yes(persist));
    } else if (cmd == "update_telemetry") {
        if (!keys(root, {"id", "cmd", "drives"}) || !parse_drives(get("drives"), runtime.host, now))
            return invalid();
        success();
        output(",\"count\":%u", runtime.host.count);
    } else if (cmd == "config_set") {
        auto candidate = runtime.settings.config;
        uint32_t version{};
        if (!keys(root, {"id", "cmd", "schema_version", "config"}) ||
            !integer(get("schema_version"), schema_version, schema_version, version) ||
            !config_patch(get("config"), candidate) || !runtime.set_config(candidate))
            return invalid();
        success();
        output(",\"config_revision\":%u", unsigned(runtime.config_revision));
    } else if (cmd == "stream_set") {
        bool enabled{};
        uint32_t interval = 1000;
        Format format{};
        if (!keys(root, {"id", "cmd", "enabled", "format", "interval_ms"}) ||
            !boolean(get("enabled"), enabled) || !parse_format(get("format"), format) ||
            (get("interval_ms") && !integer(get("interval_ms"), 250, 60000, interval)))
            return invalid();
        stream = enabled;
        interval_ms = interval;
        stream_format = format;
        last_stream = now;
        success();
        output(",\"enabled\":%s,\"interval_ms\":%u", yes(stream), unsigned(interval));
    } else if (cmd == "get_sensors") {
        Format format{};
        if (!keys(root, {"id", "cmd", "format"}) || !parse_format(get("format"), format)) return invalid();
        success();
        output(",\"sensors\":");
        sensors(now, format);
    } else {
        if (!keys(root, {"id", "cmd"})) return invalid();
        if (cmd == "get_info") {
            success();
            output(",\"hardware\":\"CMFC001\",\"board\":\"seeed_xiao_rp2350\","
                   "\"version\":\"%s\",\"revision\":\"%s\",\"protocol\":%u,"
                   "\"schema_version\":%u,\"receive_limit\":%zu,\"transmit_buffer\":%zu",
                   CMFC_VERSION, CMFC_REVISION, protocol_version, schema_version, receive_limit,
                   transmit_limit);
        } else if (cmd == "get_status") {
            success();
            output(",\"status\":");
            status(now);
        } else if (cmd == "config_get") {
            success();
            output(",\"active\":");
            config_object(*this, runtime.settings.config);
            output(",\"saved\":");
            config_object(*this, runtime.storage.saved().config);
            output(",\"active_operating\":");
            operating(*this, runtime.settings.operating);
            output(",\"saved_operating\":");
            operating(*this, runtime.storage.saved().operating);
            output(",\"saved_record_valid\":%s,\"generation\":%u", yes(runtime.storage.valid_record()),
                   unsigned(runtime.storage.generation()));
        } else if (cmd == "config_save") {
            if (!stored(runtime.save_config())) return;
            success();
            output(",\"persisted\":true");
        } else if (cmd == "config_reload") {
            runtime.reload_config();
            success();
        } else if (cmd == "config_reset") {
            if (!stored(runtime.factory_reset())) return;
            success();
            output(",\"persisted\":true");
        } else if (cmd == "clear_faults") {
            runtime.latched = 0;
            success();
        } else
            return error(identifier, "unknown_command");
    }
    finish();
}

void Protocol::status(Tick now) {
    const auto &c = runtime.settings.config;
    const auto &control = runtime.controller;
    const auto &result = control.result;
    const auto &host = runtime.host;
    output("{\"boot_id\":\"%016llx\",\"uptime_ms\":%llu,\"requested\":",
           static_cast<unsigned long long>(runtime.boot_id), static_cast<unsigned long long>(now));
    operating(*this, runtime.settings.operating);
    output(",\"applied_pwm\":[%u,%u],\"reason\":\"%s\",\"auto_policy\":\"UNCALIBRATED\","
           "\"auto_override\":%s,\"power_state\":\"%s\",\"power_detection_validated\":%s,"
           "\"power_confidence\":\"%s\",\"config_revision\":%u,\"calibration_revision\":%u,"
           "\"storage_generation\":%u,\"stream_dropped\":%u,"
           "\"host_connected\":%s,\"host_eligible\":%s,\"host_expected_ms\":%u,"
           "\"host_message_age_ms\":",
           unsigned(result.output.pwm[0]), unsigned(result.output.pwm[1]), name(result.output.reason),
           yes(result.output.reason == Reason::Protective || result.output.reason == Reason::Startup),
           name(control.power.state), yes(c.power_detection_validated),
           control.power.state == PowerState::Unknown    ? "NONE"
           : control.power.state == PowerState::Starting ? "PROVISIONAL"
                                                         : "INFERRED",
           unsigned(runtime.config_revision), unsigned(runtime.calibration_revision),
           unsigned(runtime.storage.generation()), unsigned(dropped), yes(host.connected),
           yes(host.eligible(now, c)), unsigned(c.host_expected_ms));
    time_or_null(*this, host.seen, now - host.received_at);
    output(",\"host_drives\":[");
    for (unsigned i = 0; i < host.count; ++i) {
        const auto &drive = host.drives[i];
        const bool has_state = drive.spindle != Spindle::Unknown || drive.io != Activity::Unknown ||
                               drive.self_test != Activity::Unknown;
        const auto age = host.state_age(drive, now);
        output("%s{\"drive_id\":\"%s\",\"temperature_c\":", i ? "," : "", drive.id.data());
        nullable(*this, drive.temperature_valid, drive.celsius);
        output(",\"temperature_age_ms\":");
        time_or_null(*this, drive.temperature_valid, host.temperature_age(drive, now));
        output(",\"temperature_freshness\":\"%s\",\"spindle\":\"%s\",\"io\":\"%s\","
               "\"self_test\":\"%s\",\"state_freshness\":\"%s\",\"state_age_ms\":",
               name(host.freshness(drive, now, c)), name(drive.spindle), name(drive.io),
               name(drive.self_test),
               name(!has_state                ? Freshness::Missing
                    : age >= c.host_expire_ms ? Freshness::Expired
                    : age > c.host_fresh_ms   ? Freshness::Aging
                                              : Freshness::Fresh));
        time_or_null(*this, has_state, age);
        output("}");
    }
    output("],\"diagnostics\":[");
    bool first = true;
    for (const auto &entry : diagnostics) {
        if (!(result.diagnostics & entry.mask)) continue;
        output("%s{\"code\":\"%s\",\"severity\":\"%s\"}", first ? "" : ",", entry.code, entry.severity);
        first = false;
    }
    output("]}");
}

void Protocol::sensors(Tick now, Format format) {
    const bool raw = format != Format::Physical;
    const bool physical = format != Format::Raw;
    const auto &s = runtime.sensors;
    const auto &e = runtime.controller.result;
    const auto &c = runtime.settings.config;
    const auto &ina = s.ina;
    const auto ina_error = now - ina.at <= sensor_fresh_ms ? e.electrical.error : SampleError::Missing;
    const bool ina_valid = ina_error == SampleError::None;
    output("{\"ina\":{\"valid\":%s,\"error\":\"%s\",\"calibrated\":%s,\"at_ms\":", yes(ina_valid),
           name(ina_error), yes(c.ina_calibrated));
    time_or_null(*this, ina.at || ina.raw_valid, ina.at);
    if (raw) {
        output(",\"config\":%u,\"adc_config\":%u,\"diag\":", unsigned(ina_config), unsigned(ina_adc_config));
        nullable(*this, ina.diag_valid, ina.diag);
        output(",\"bus_raw\":");
        nullable(*this, ina.raw_valid, ina.bus_raw);
        output(",\"shunt_raw\":");
        nullable(*this, ina.raw_valid, ina.shunt_raw);
    }
    if (physical) {
        output(",\"bus_v\":");
        nullable(*this, ina_valid, e.electrical.bus_v);
        output(",\"shunt_v\":");
        nullable(*this, ina_valid, e.electrical.shunt_v);
        output(",\"current_a\":");
        nullable(*this, ina_valid, e.electrical.current_a);
        output(",\"power_w\":");
        nullable(*this, ina_valid, e.electrical.power_w);
    }
    output("},\"fans\":[");
    for (unsigned i = 0; i < fan_count; ++i) {
        const auto &adc = s.adc[i];
        const auto &t = e.thermistors[i];
        const auto &tach = s.tach[i];
        const auto thermistor_error = now - adc.at <= sensor_fresh_ms ? t.error : SampleError::Missing;
        const bool valid_t = thermistor_error == SampleError::None;
        const bool valid_tach = tach.window_ms && now - tach.at <= tach_fresh_ms;
        const auto ppr = i ? c.lower_ppr : c.upper_ppr;
        output("%s{\"position\":\"%s\",\"plenum\":{\"valid\":%s,\"error\":\"%s\","
               "\"calibrated\":%s,\"at_ms\":",
               i ? "," : "", i ? "lower" : "upper", yes(valid_t), name(thermistor_error),
               yes(i ? c.lower_ntc_calibrated : c.upper_ntc_calibrated));
        time_or_null(*this, adc.present, adc.at);
        if (raw) {
            output(",\"adc_count\":");
            nullable(*this, adc.present, adc.count);
        }
        if (physical) {
            output(",\"ohm\":");
            nullable(*this, valid_t, t.ohm);
            output(",\"temperature_c\":");
            nullable(*this, valid_t, t.celsius);
        }
        output("},\"tach\":{\"valid\":%s,\"ppr\":%u,\"at_ms\":", yes(valid_tach), unsigned(ppr));
        time_or_null(*this, tach.window_ms, tach.at);
        output(",\"last_pulse_age_ms\":");
        time_or_null(*this, tach.saw_pulse, now - tach.last_pulse);
        if (raw) {
            output(",\"pulses\":");
            nullable(*this, tach.window_ms, tach.pulses);
            output(",\"window_ms\":");
            time_or_null(*this, tach.window_ms, tach.window_ms);
        }
        if (physical) {
            output(",\"rpm\":");
            nullable(*this, valid_tach, tach.rpm(ppr));
        }
        output("}}");
    }
    output("]}");
}
} // namespace cmfc
