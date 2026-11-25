#pragma once

#include "model.hpp"
#include "storage.hpp"

namespace cmfc::pico {
Tick now_ms();
void init_pwm(const Output &initial);
void apply_pwm(const Output &output);
void start_acquisition(Tick now);
bool acquire(Tick now, Snapshot &latest);
StorageBackend flash_storage();
} // namespace cmfc::pico
