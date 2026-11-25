#include "runtime.hpp"

namespace cmfc {
void Runtime::load() {
    storage.load();
    settings = storage.saved();
    if (storage.bad_record()) latched |= ConfigInvalid;
}

void Runtime::evaluate(Tick now) {
    controller.evaluate(now, settings, sensors, host,
                        latched | (watchdog_reset ? uint32_t{WatchdogReset} : 0u));
}

bool Runtime::save(const Settings &candidate) {
    if (storage.save(candidate)) return true;
    latched |= StorageFailure;
    return false;
}

bool Runtime::set_operating(const Operating &operating, bool persist) {
    if (!valid(operating)) return false;
    settings.operating = operating;
    return !persist || save({storage.saved().config, operating});
}

bool Runtime::set_config(const Config &config) {
    if (!valid(config)) return false;
    const auto &old = settings.config;
    const bool calibration_changed =
        old.shunt_ohm != config.shunt_ohm || old.current_gain != config.current_gain ||
        old.current_offset_a != config.current_offset_a || old.voltage_gain != config.voltage_gain ||
        old.voltage_offset_v != config.voltage_offset_v || old.ina_calibrated != config.ina_calibrated ||
        old.upper_r0_ohm != config.upper_r0_ohm || old.upper_t0_c != config.upper_t0_c ||
        old.upper_beta_k != config.upper_beta_k || old.upper_divider_ohm != config.upper_divider_ohm ||
        old.upper_offset_c != config.upper_offset_c || old.upper_adc_gain != config.upper_adc_gain ||
        old.upper_ntc_calibrated != config.upper_ntc_calibrated || old.lower_r0_ohm != config.lower_r0_ohm ||
        old.lower_t0_c != config.lower_t0_c || old.lower_beta_k != config.lower_beta_k ||
        old.lower_divider_ohm != config.lower_divider_ohm || old.lower_offset_c != config.lower_offset_c ||
        old.lower_adc_gain != config.lower_adc_gain ||
        old.lower_ntc_calibrated != config.lower_ntc_calibrated;
    if (calibration_changed) ++calibration_revision;
    if (!(old == config)) {
        ++config_revision;
        controller.power = {};
    }
    settings.config = config;
    return true;
}

bool Runtime::save_config() { return save({settings.config, storage.saved().operating}); }

void Runtime::reload_config() { set_config(storage.saved().config); }

bool Runtime::factory_reset() {
    const Settings defaults{};
    set_config(defaults.config);
    settings.operating = defaults.operating;
    return save(defaults);
}
} // namespace cmfc
