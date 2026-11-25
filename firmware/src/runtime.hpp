#pragma once

#include "model.hpp"
#include "storage.hpp"

namespace cmfc {
struct Runtime {
    explicit Runtime(StorageBackend backend) : storage(backend) {}
    Storage storage;
    Settings settings{};
    Snapshot sensors;
    Host host;
    Controller controller;
    uint32_t config_revision{}, calibration_revision{}, latched{};
    uint64_t boot_id{};
    bool watchdog_reset{};

    void load();
    void evaluate(Tick now);
    bool set_operating(const Operating &operating, bool persist);
    bool set_config(const Config &config);
    bool save_config();
    void reload_config();
    bool factory_reset();

  private:
    bool save(const Settings &candidate);
};
} // namespace cmfc
