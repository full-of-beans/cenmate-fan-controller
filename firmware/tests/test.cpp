#include "protocol.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>

#include "yyjson.h"

using namespace cmfc;

static const char *test_case = "";
static unsigned checks{};
#define REQUIRE(...)                                                                                         \
    do {                                                                                                     \
        ++checks;                                                                                            \
        if (!(__VA_ARGS__)) {                                                                                \
            std::cerr << test_case << " line " << __LINE__ << ": " << #__VA_ARGS__ << '\n';                  \
            std::exit(1);                                                                                    \
        }                                                                                                    \
    } while (false)

static bool near(float a, float b, float tolerance = 0.001f) { return std::abs(a - b) <= tolerance; }

struct FakeFlash {
    std::array<Page, 2> pages{};
    unsigned writes{};
    bool cut{};
    FakeFlash() {
        for (auto &page : pages)
            page.fill(0xff);
    }
    StorageBackend backend() { return {this, read, write}; }
    static bool read(void *context, unsigned slot, Page &page) {
        page = static_cast<FakeFlash *>(context)->pages.at(slot);
        return true;
    }
    static bool write(void *context, unsigned slot, const Page &page) {
        auto &memory = *static_cast<FakeFlash *>(context);
        ++memory.writes;
        memory.pages.at(slot).fill(0xff);
        if (memory.cut) {
            std::copy_n(page.begin(), 100, memory.pages.at(slot).begin());
            return false;
        }
        memory.pages.at(slot) = page;
        return true;
    }
};

static uint32_t raw(int32_t count) { return (static_cast<uint32_t>(count) & 0xfffff) << 4; }

static Snapshot normal(Tick now, float bus = 12, float current = 1) {
    Snapshot s;
    s.ina = {now,
             SampleError::None,
             true,
             true,
             raw(std::lround(bus / 0.0001953125f)),
             raw(std::lround(current / 0.000000078125f * 0.001f)),
             3};
    for (auto &adc : s.adc)
        adc = {now, true, 2048};
    for (auto &tach : s.tach)
        tach = {now, 1000, now, 80, true};
    return s;
}

static std::array<Drive, drive_capacity> drives(float temp = 35, uint32_t age = 0) {
    std::array<Drive, drive_capacity> values{};
    for (unsigned i = 0; i < values.size(); ++i) {
        values[i].id = {'d', static_cast<char>('0' + i)};
        values[i].temperature_valid = true;
        values[i].celsius = temp;
        values[i].temperature_age_ms = age;
    }
    return values;
}

static void control() {
    Controller c;
    Settings cfg;
    Host host;
    c.evaluate(0, cfg, normal(0), host, 0);
    REQUIRE(cfg.operating.mode == Mode::Auto);
    REQUIRE(c.result.diagnostics & Uncalibrated);
    REQUIRE(c.result.output.pwm == std::array<uint8_t, 2>{100, 100});

    cfg.config.power_detection_validated = 1;
    cfg.config.auto_fallback_pwm = 80;
    cfg.config.plenum_limit_enabled = 1;
    cfg.config.plenum_emergency_c = 40;
    cfg.config.startup_boost_ms = 2000;
    cfg.config.stall_min_pwm = 1;
    cfg.operating = {Mode::Fixed, {0, 100}};

    auto fixed = [&](Tick at, Snapshot sensors, uint32_t diagnostic, PowerState state) {
        c.evaluate(at, cfg, sensors, host, 0);
        REQUIRE(c.result.output.pwm == std::array<uint8_t, 2>{0, 100});
        REQUIRE(c.result.output.reason == Reason::Fixed);
        REQUIRE(c.power.state == state);
        REQUIRE((c.result.diagnostics & diagnostic) == diagnostic);
    };
    fixed(1, normal(1, 0, 0), 0, PowerState::Unknown);
    fixed(1001, normal(1001, 0, 0), 0, PowerState::Off);
    fixed(2001, normal(2001), Uncalibrated, PowerState::Starting);
    fixed(3001, normal(3001), Uncalibrated, PowerState::On);
    const auto &limits = cfg.config;
    host.connected = true;
    REQUIRE(host.update(drives(42), 1, 3100));
    fixed(3100, normal(3100), HddHot, PowerState::On);
    auto hot_plenum = normal(3200);
    hot_plenum.adc[0].count = 1200;
    fixed(3200, hot_plenum, PlenumHot, PowerState::On);
    auto missing_tach = normal(3300);
    missing_tach.tach[1] = {};
    fixed(3300, missing_tach, LowerUnconfirmed, PowerState::On);
    for (Tick at : {9000u, 13000u}) {
        auto zero_tach = normal(at);
        zero_tach.tach[1] = {at, 1000, 0, 0, false};
        fixed(at, zero_tach, at == 13000 ? LowerStall : LowerUnconfirmed, PowerState::On);
    }
    host.connected = false;
    fixed(13100, normal(13100), HostUnavailable, PowerState::On);
    fixed(3100 + limits.host_expire_ms, normal(3100 + limits.host_expire_ms), HostUnavailable | HddHot,
          PowerState::On);
    auto failed = normal(300000);
    failed.ina.error = SampleError::Communication;
    failed.adc[1].count = 4095;
    fixed(300000, failed, InaCommunication | LowerNtc, PowerState::Unknown);

    cfg.operating.mode = Mode::Auto;
    c.evaluate(300100, cfg, failed, host, 0);
    REQUIRE(c.result.output.reason == Reason::Protective);
    REQUIRE(c.result.output.pwm == std::array<uint8_t, 2>{100, 100});
    REQUIRE(cfg.operating.pwm == std::array<uint8_t, 2>{0, 100});
    host.connected = true;
    REQUIRE(host.update(drives(39), 1, 300200));
    c.evaluate(300200, cfg, normal(300200), host, 0);
    REQUIRE(c.result.diagnostics & HddHot);
    REQUIRE(host.update(drives(37), 1, 300300));
    c.evaluate(300300, cfg, normal(300300), host, 0);
    REQUIRE(!(c.result.diagnostics & HddHot));
    REQUIRE(c.result.output.reason == Reason::Protective);
    c.evaluate(303299, cfg, normal(303299), host, 0);
    REQUIRE(c.result.output.reason == Reason::Protective);
    c.evaluate(306000, cfg, normal(306000), host, 0);
    REQUIRE(c.result.output.reason == Reason::Uncalibrated);
    REQUIRE(c.result.output.pwm == std::array<uint8_t, 2>{80, 80});
    REQUIRE(!valid(Operating{Mode::Fixed, {101, 50}}));

    Controller alarm;
    cfg.operating.mode = Mode::Auto;
    Host optional_host;
    alarm.evaluate(0, cfg, normal(0), optional_host, 0);
    alarm.evaluate(1000, cfg, normal(1000), optional_host, 0);
    REQUIRE(alarm.result.output.reason == Reason::Startup);
    auto neither_rotates = normal(10000);
    neither_rotates.tach = {};
    alarm.evaluate(10000, cfg, neither_rotates, optional_host, 0);
    REQUIRE(!(alarm.result.diagnostics & (UpperStall | LowerStall)));
    for (Tick at : {11000u, 15000u}) {
        auto one_rotates = normal(at);
        one_rotates.tach[1] = {at, 1000, 0, 0, false};
        alarm.evaluate(at, cfg, one_rotates, optional_host, 0);
    }
    REQUIRE(alarm.result.diagnostics & LowerStall);
    REQUIRE(alarm.result.output.reason == Reason::Protective);
}

static void sensors() {
    Config c;
    auto s = normal(10);
    REQUIRE(near(convert(s.ina, c).bus_v, 12));
    REQUIRE(near(convert(s.ina, c).current_a, 1));
    REQUIRE(near(convert(s.ina, c).power_w, 12));
    s.ina.shunt_raw = raw(-12800);
    REQUIRE(near(convert(s.ina, c).shunt_v, -0.001f, 0.0000001f));
    REQUIRE(near(convert(s.ina, c, false).current_a, -4));
    c.current_gain = 1.5f;
    c.current_offset_a = 0.5f;
    REQUIRE(near(convert(s.ina, c).current_a, -1));
    s.ina.shunt_raw = raw(-524288);
    REQUIRE(convert(s.ina, c).error == SampleError::Overflow);
    s.ina = normal(0).ina;
    s.ina.diag = 0;
    REQUIRE(convert(s.ina, c).error == SampleError::Trim);
    s.ina.diag = 0x203;
    REQUIRE(convert(s.ina, c).error == SampleError::Overflow);
    s.ina.diag = 3;
    s.ina.bus_raw = raw(-1);
    REQUIRE(convert(s.ina, c).error == SampleError::Invalid);
    auto t = convert(s.adc[0], c, 0);
    REQUIRE(near(t.ohm, 10000, 5));
    REQUIRE(near(t.celsius, 25, 0.02f));
    c.lower_offset_c = 2;
    REQUIRE(near(convert(s.adc[1], c, 1).celsius, 27, 0.02f));
    REQUIRE(convert(Adc{0, true, 4095}, c, 0).error == SampleError::Open);
    REQUIRE(convert(Adc{0, true, 0}, c, 0).error == SampleError::Short);
    REQUIRE(convert(Adc{0, true, 6000}, c, 0).error == SampleError::Invalid);
    REQUIRE(convert(Adc{}, c, 0).error == SampleError::Missing);
    REQUIRE(pulse_delta(2, UINT32_MAX - 1) == 4);
    REQUIRE(near(Tach{0, 1200, 0, 100, true}.rpm(2), 2500));
    REQUIRE(near(Tach{0, 1000, 0, 0, false}.rpm(2), 0));
}

static void host() {
    Host host;
    Config c;
    host.connected = true;
    auto d = drives();
    REQUIRE(host.update(d, 6, 100));
    REQUIRE(host.eligible(101, c));
    REQUIRE(host.count == 6);
    REQUIRE(!host.update(d, 7, 101));
    d[1] = d[0];
    REQUIRE(!host.update(d, 2, 102));
    d[0].id[0] = '"';
    REQUIRE(!host.update(d, 1, 103));
    REQUIRE(host.received_at == 100);
    d = drives(37, c.host_fresh_ms + 1);
    REQUIRE(host.update(d, 2, 1000));
    REQUIRE(host.freshness(d[0], 1000, c) == Freshness::Aging);
    REQUIRE(!host.eligible(1000, c));
    REQUIRE(host.freshness(d[0], 1000 + c.host_expire_ms, c) == Freshness::Expired);
    d[0].temperature_valid = false;
    d[1].temperature_age_ms = 0;
    REQUIRE(host.update(d, 2, 1000));
    REQUIRE(host.freshness(d[0], 1000, c) == Freshness::Missing);
    REQUIRE(host.freshness(d[1], 1000, c) == Freshness::Fresh);
    REQUIRE(!host.eligible(1000, c));
    REQUIRE(host.drives[0].spindle == Spindle::Unknown);
    REQUIRE(host.update(drives(), 1, 2000));
    host.connected = false;
    REQUIRE(!host.eligible(2001, c));
    REQUIRE(host.freshness(host.drives[0], 2001, c) == Freshness::Fresh);
}

static void power() {
    Config c;
    Power power;
    auto reading = Electrical{SampleError::None, 12, 0.001f, 1, 12};
    power.update(0, reading, c);
    power.update(2000, reading, c);
    REQUIRE(power.state == PowerState::Unknown);
    c.power_detection_validated = 1;
    power.update(2100, reading, c);
    REQUIRE(power.state == PowerState::Starting);
    power.update(2200, reading, c);
    REQUIRE(power.state == PowerState::Starting);
    power.update(3100, reading, c);
    REQUIRE(power.state == PowerState::On);
    reading.current_a = 0.05f;
    power.update(3200, reading, c);
    REQUIRE(power.state == PowerState::On);
    reading.current_a = 0;
    power.update(3300, reading, c);
    REQUIRE(power.state == PowerState::Unknown);
    reading.bus_v = 0;
    power.update(3400, reading, c);
    REQUIRE(power.state == PowerState::Unknown);
    power.update(4400, reading, c);
    REQUIRE(power.state == PowerState::Off);
    power.update(4500, {}, c);
    REQUIRE(power.state == PowerState::Unknown);
    reading.current_a = -1;
    power.update(5500, reading, c);
    REQUIRE(power.state == PowerState::Unknown);
}

static void storage() {
    FakeFlash flash;
    Runtime runtime(flash.backend());
    runtime.load();
    REQUIRE(runtime.settings == Settings{});
    REQUIRE(!runtime.storage.bad_record());
    auto candidate = runtime.settings.config;
    candidate.upper_ppr = 4;
    REQUIRE(runtime.set_config(candidate));
    candidate.host_fresh_ms = 5;
    REQUIRE(!runtime.set_config(candidate));
    REQUIRE(runtime.settings.config.upper_ppr == 4);

    const Operating fixed{Mode::Fixed, {0, 100}};
    REQUIRE(runtime.set_operating(fixed, true));
    REQUIRE(runtime.storage.saved().config.upper_ppr == 2);
    REQUIRE(runtime.storage.saved().operating == fixed);
    REQUIRE(runtime.set_operating(fixed, true));
    REQUIRE(flash.writes == 1);
    REQUIRE(runtime.set_operating({Mode::Fixed, {49, 51}}, false));
    REQUIRE(runtime.save_config());
    REQUIRE(runtime.storage.saved().operating == fixed);
    REQUIRE(runtime.storage.saved().config.upper_ppr == 4);
    REQUIRE(flash.writes == 2);
    Runtime reboot(flash.backend());
    reboot.load();
    REQUIRE(reboot.settings.operating == fixed);
    REQUIRE(reboot.settings.config.upper_ppr == 4);

    flash.cut = true;
    REQUIRE(!reboot.set_operating({Mode::Auto, {100, 100}}, true));
    REQUIRE(reboot.settings.operating.mode == Mode::Auto);
    REQUIRE(reboot.latched & StorageFailure);
    reboot.load();
    REQUIRE(reboot.storage.bad_record());
    REQUIRE(reboot.settings.operating == fixed);
    REQUIRE(reboot.settings.config.upper_ppr == 4);

    flash.cut = false;
    REQUIRE(reboot.factory_reset());
    REQUIRE(reboot.settings == Settings{});
    reboot.load();
    REQUIRE(reboot.settings == Settings{});
    // A CRC-valid record of an unsupported schema still falls back to the prior record.
    auto &newest = flash.pages[0];
    const auto intact = newest;
    newest[4] = 2;
    uint32_t crc = 0xffffffff;
    for (unsigned i = 0; i < record_size - 4; ++i) {
        crc ^= newest[i];
        for (unsigned bit = 0; bit < 8; ++bit)
            crc = (crc >> 1) ^ ((0u - (crc & 1u)) & 0xedb88320u);
    }
    crc = ~crc;
    for (unsigned i = 0; i < 4; ++i)
        newest[record_size - 4 + i] = static_cast<uint8_t>(crc >> (i * 8));
    reboot.load();
    REQUIRE(reboot.storage.bad_record());
    REQUIRE(reboot.settings.operating == fixed);
    newest = intact;
    // Damage the newest record and fall back to the older valid generation.
    flash.pages[0][90] ^= 1;
    reboot.load();
    REQUIRE(reboot.storage.bad_record());
    REQUIRE(reboot.settings.operating == fixed);
    flash.pages[1][10] ^= 1;
    reboot.load();
    REQUIRE(reboot.settings == Settings{});
    REQUIRE(!reboot.storage.valid_record());
}

static void protocol() {
    FakeFlash flash;
    Runtime runtime(flash.backend());
    runtime.load();
    Protocol p(runtime);
    p.connection(true);

    const auto drain = [&] {
        std::string message(p.pending());
        REQUIRE(!message.empty());
        REQUIRE(message.back() == '\n');
        p.consume(message.size());
        auto *doc = yyjson_read(message.data(), message.size(), 0);
        REQUIRE(doc && yyjson_is_obj(yyjson_doc_get_root(doc)));
        yyjson_doc_free(doc);
        return message;
    };
    const auto send = [&](std::string_view request, Tick now = 0) {
        bool accepted = true;
        for (char ch : request)
            accepted &= p.feed(ch, now);
        REQUIRE(accepted);
        REQUIRE(p.feed('\n', now));
        return drain();
    };
    const auto expect = [&](std::string_view request, std::string_view substring, Tick now = 0) {
        REQUIRE(send(request, now).find(substring) != std::string::npos);
    };

    expect(R"({"id":4,"cmd":"get_info"})", R"("id":4,"ok":true)");
    expect(R"({"id":4294967295,"cmd":"get_status"})", R"("id":4294967295)");
    expect(R"({"id":4294967296,"cmd":"get_status"})", "invalid_id");
    expect(R"({"id":1,"cmd":"typo"})", "unknown_command");
    expect(R"({"id":1,"cmd":)", "malformed_json");
    expect(R"({"id":1,"cmd":"get_info","cmd":"get_status"})", "invalid_request");
    expect(R"({"id":1,"cmd":"set_pwm","upper_pwm":12})", "requires_fixed");

    expect(R"({"id":3,"cmd":"config_set","schema_version":2,"config":{"lower_ppr":4}})", "invalid_request");
    expect(R"({"id":3,"cmd":"config_set","schema_version":1,"config":{"lower_ppr":3}})", R"("ok":true)");
    expect(R"({"id":3,"cmd":"config_set","schema_version":1,"config":{"lower_ppr":4,"shunt_ohm":1e999}})",
           R"("ok":false)");
    REQUIRE(runtime.settings.config.lower_ppr == 3);
    expect(R"({"id":3,"cmd":"config_set","schema_version":1,"config":{"lower_ppr":4,"lower_ppr":5}})",
           "invalid_request");
    expect(R"({"id":3,"cmd":"config_set","schema_version":1,"config":{"lower_ppr":4,"hdd_emergency_c":34}})",
           "invalid_request");
    REQUIRE(runtime.settings.config.lower_ppr == 3);
    expect(R"({"id":3,"cmd":"config_get"})", R"("saved_operating")");
    expect(R"({"id":3,"cmd":"config_reload"})", R"("ok":true)");
    REQUIRE(runtime.settings.config.lower_ppr == 2);
    expect(R"({"id":3,"cmd":"set_mode","mode":"FIXED","lower_pwm":58,"upper_pwm":0})", R"("persisted":true)");
    expect(R"({"id":3,"cmd":"set_pwm","upper_pwm":999,"lower_pwm":1})", "invalid_request");
    REQUIRE(runtime.settings.operating.pwm == std::array<uint8_t, 2>{0, 58});
    expect(R"({"id":3,"cmd":"set_pwm","upper_pwm":1,"persist":false})", R"("persisted":false)");
    REQUIRE(runtime.storage.saved().operating.pwm == std::array<uint8_t, 2>{0, 58});
    REQUIRE(runtime.settings.operating.pwm == std::array<uint8_t, 2>{1, 58});
    flash.cut = true;
    expect(R"({"id":3,"cmd":"set_pwm","upper_pwm":80})", R"("error":"storage_failure","applied":true)");
    REQUIRE(runtime.settings.operating.pwm[0] == 80);
    flash.cut = false;

    expect(
        R"({"id":3,"cmd":"update_telemetry","drives":[{"drive_id":"disk:A","temperature_c":38,"temperature_age_ms":120000,"io":"NO","state_age_ms":1000},{"drive_id":"disk:B","temperature_c":null}]})",
        R"("count":2)");
    expect(R"({"id":3,"cmd":"get_status"})", R"("temperature_freshness":"AGING")");
    expect(R"({"id":3,"cmd":"update_telemetry","drives":[{"drive_id":"disk:A"},{"drive_id":"disk:A"}]})",
           "invalid_request");
    expect(R"({"id":3,"cmd":"update_telemetry","drives":[{"drive_id":"disk\u0000A"}]})", "invalid_request");
    REQUIRE(runtime.host.count == 2);

    expect(R"({"id":3,"cmd":"get_sensors","format":"PHYSICAL"})", R"("power_w":null)");
    const auto raw_response = send(R"({"id":3,"cmd":"get_sensors","format":"RAW"})");
    REQUIRE(raw_response.find("shunt_raw") != std::string::npos);
    REQUIRE(raw_response.find("power_w") == std::string::npos);

    REQUIRE(p.feed('{', 100));
    p.tick(2099);
    REQUIRE(p.pending().empty());
    p.tick(2100);
    REQUIRE(drain().find("partial_timeout") != std::string::npos);
    expect(std::string(Protocol::receive_limit + 1, ' '), "message_too_large", 2101);

    expect(R"({"id":99,"cmd":"stream_set","enabled":true,"interval_ms":249})", "invalid_request", 3000);
    expect(R"({"id":99,"cmd":"stream_set","enabled":true,"interval_ms":250})", R"("enabled":true)", 3000);
    p.tick(3249);
    REQUIRE(p.pending().empty());
    p.tick(3250, false);
    REQUIRE(p.pending().empty());
    p.tick(3500);
    REQUIRE(!p.pending().empty());
    REQUIRE(!p.feed('{', 3500));
    const auto size = p.pending().size();
    p.consume(1);
    p.tick(3750);
    REQUIRE(p.pending().size() == size - 1);
    REQUIRE(p.pending().find(R"("event":"telemetry")") != std::string::npos);
    p.consume(p.pending().size());
    expect(R"({"id":4,"cmd":"get_status"})", R"("stream_dropped":2)", 3751);
    expect(R"({"id":99,"cmd":"stream_set","enabled":false})", R"("enabled":false)", 3752);
    p.tick(5000);
    REQUIRE(p.pending().empty());

    REQUIRE(p.feed('{', 5000));
    p.connection(false);
    REQUIRE(!p.feed('}', 5001));
    REQUIRE(!runtime.host.connected);
    p.connection(true);
    expect(R"({"id":3,"cmd":"config_reset"})", R"("persisted":true)", 5010);
    REQUIRE(runtime.settings == Settings{});
    expect(R"({"id":3,"cmd":"clear_faults"})", R"("ok":true)", 5020);
    REQUIRE(runtime.latched == 0);
}

int main(int argc, char **argv) {
    const struct {
        const char *name;
        void (*fn)();
    } suites[]{
        {"control", control}, {"sensors", sensors}, {"host", host},
        {"power", power},     {"storage", storage}, {"protocol", protocol},
    };
    if (argc != 2) return 2;
    for (const auto &suite : suites) {
        if (std::string_view(argv[1]) != suite.name) continue;
        test_case = suite.name;
        suite.fn();
        std::cout << test_case << ": " << checks << " checks passed\n";
        return 0;
    }
    return 2;
}
