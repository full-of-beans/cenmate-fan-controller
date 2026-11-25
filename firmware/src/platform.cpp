#include "platform.hpp"

#include <cstring>

#include "hardware/adc.h"
#include "hardware/clocks.h"
#include "hardware/flash.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "hardware/pwm.h"
#include "hardware/regs/addressmap.h"
#include "hardware/sync.h"
#include "pico/flash.h"
#include "pico/time.h"

#include "build_info.hpp"

namespace cmfc::pico {
namespace {
constexpr std::array<unsigned, fan_count> pwm_pins{2, 4};
constexpr std::array<unsigned, fan_count> tach_pins{1, 3};
constexpr std::array<unsigned, fan_count> adc_pins{26, 27};
constexpr unsigned sda = 6, scl = 7;
constexpr unsigned pwm_hz = 25000;
constexpr unsigned bus_hz = 100000;
constexpr unsigned transaction_us = 2000;
constexpr Tick sample_interval = 100, tach_interval = 1000;
constexpr Tick conversion_check_ms = 36, conversion_deadline_ms = 70;
constexpr uint8_t reg_config = 0x00, reg_adc_config = 0x01, reg_shunt_cal = 0x02;
constexpr uint8_t reg_shunt = 0x04, reg_bus = 0x05, reg_diag = 0x0b;
constexpr uint8_t reg_mfr_id = 0x3e, reg_device_id = 0x3f;
constexpr uint16_t ti_id = 0x5449, ina228_id = 0x228;
constexpr uint32_t physical_flash = 2u * 1024 * 1024;
constexpr uint32_t storage_offset = CMFC_STORAGE_OFFSET;

static_assert(record_size == FLASH_PAGE_SIZE);
static_assert(storage_offset + 2 * FLASH_SECTOR_SIZE == physical_flash);
static_assert(storage_offset % FLASH_SECTOR_SIZE == 0);
static_assert(PICO_FLASH_SIZE_BYTES == physical_flash);

uint16_t pwm_period{};
Tick next_adc{}, tach_start{};
volatile uint32_t pulse_counts[fan_count]{};
volatile Tick pulse_times[fan_count]{};
volatile bool ever_pulsed[fan_count]{};
std::array<uint32_t, fan_count> previous_counts{};
bool flash_interrupted_tach{};

void tach_irq(unsigned gpio, uint32_t events) {
    if (!(events & GPIO_IRQ_EDGE_FALL)) return;
    const unsigned fan = gpio == tach_pins[1] ? 1 : 0;
    if (gpio != tach_pins[fan]) return;
    pulse_counts[fan] = pulse_counts[fan] + 1;
    pulse_times[fan] = now_ms();
    ever_pulsed[fan] = true;
}

void reset_i2c() {
    i2c_deinit(i2c1);
    // Open-drain bus clear. If the clock is held low, leave the bus for the next retry.
    gpio_init(sda);
    gpio_init(scl);
    gpio_put(sda, false);
    gpio_put(scl, false);
    for (unsigned pulse = 0; pulse < 9 && gpio_get(scl) && !gpio_get(sda); ++pulse) {
        gpio_set_dir(scl, GPIO_OUT);
        busy_wait_us_32(5);
        gpio_set_dir(scl, GPIO_IN);
        busy_wait_us_32(5);
    }
    if (gpio_get(scl) && gpio_get(sda)) {
        gpio_set_dir(scl, GPIO_OUT);
        gpio_set_dir(sda, GPIO_OUT);
        busy_wait_us_32(5);
        gpio_set_dir(scl, GPIO_IN);
        busy_wait_us_32(5);
        gpio_set_dir(sda, GPIO_IN);
    }
    i2c_init(i2c1, bus_hz);
    gpio_set_function(sda, GPIO_FUNC_I2C);
    gpio_set_function(scl, GPIO_FUNC_I2C);
}

bool read_adc(uint16_t &count) {
    const auto deadline = time_us_64() + 100;
    hw_set_bits(&adc_hw->cs, ADC_CS_START_ONCE_BITS);
    while (!(adc_hw->cs & ADC_CS_READY_BITS)) {
        if (time_us_64() >= deadline) return false;
        tight_loop_contents();
    }
    count = static_cast<uint16_t>(adc_hw->result);
    return true;
}

bool read_reg(uint8_t reg, unsigned length, uint32_t &value) {
    uint8_t bytes[3]{};
    const auto deadline = make_timeout_time_us(transaction_us);
    if (i2c_write_blocking_until(i2c1, ina_address, &reg, 1, true, deadline) != 1 ||
        i2c_read_blocking_until(i2c1, ina_address, bytes, length, false, deadline) !=
            static_cast<int>(length))
        return false;
    value = 0;
    for (unsigned i = 0; i < length; ++i)
        value = (value << 8) | bytes[i];
    return true;
}

bool write_reg(uint8_t reg, uint16_t value) {
    const uint8_t bytes[]{reg, static_cast<uint8_t>(value >> 8), static_cast<uint8_t>(value)};
    return i2c_write_blocking_until(i2c1, ina_address, bytes, 3, false,
                                    make_timeout_time_us(transaction_us)) == 3;
}

struct InaAcquisition {
    bool configured{}, converting{};
    Tick next{}, started{};

    bool fail(Tick now, Ina &sample, SampleError error) {
        sample = {now, error};
        configured = converting = false;
        next = now + sample_interval;
        reset_i2c();
        return true;
    }

    bool step(Tick now, Ina &sample) {
        if (now < next) return false;
        if (!converting) {
            if (!configured) {
                uint32_t manufacturer{}, device{};
                if (!read_reg(reg_mfr_id, 2, manufacturer) || !read_reg(reg_device_id, 2, device))
                    return fail(now, sample, SampleError::Communication);
                if (manufacturer != ti_id || device >> 4 != ina228_id)
                    return fail(now, sample, SampleError::WrongDevice);
                if (!write_reg(reg_config, ina_config) || !write_reg(reg_shunt_cal, 0))
                    return fail(now, sample, SampleError::Communication);
                configured = true;
            }
            if (!write_reg(reg_adc_config, ina_adc_config))
                return fail(now, sample, SampleError::Communication);
            started = now_ms();
            next = started + conversion_check_ms;
            converting = true;
            return false;
        }

        uint32_t diag{};
        if (!read_reg(reg_diag, 2, diag)) return fail(now, sample, SampleError::Communication);
        if (!(diag & 2u)) {
            if (now - started >= conversion_deadline_ms) {
                fail(now, sample, SampleError::ConversionTimeout);
                sample.diag_valid = true;
                sample.diag = static_cast<uint16_t>(diag);
                return true;
            }
            next = now + 5;
            return false;
        }

        uint32_t config{}, bus{}, shunt{};
        if (!read_reg(reg_bus, 3, bus) || !read_reg(reg_shunt, 3, shunt) || !read_reg(reg_config, 2, config))
            return fail(now, sample, SampleError::Communication);
        if (config != ina_config) return fail(now, sample, SampleError::Invalid);
        sample = {now_ms(), SampleError::None, true, true, bus, shunt, static_cast<uint16_t>(diag)};
        next = started + sample_interval;
        converting = false;
        return true;
    }
} ina_acquisition;

bool read_flash(void *, unsigned slot, Page &page) {
    const auto address = XIP_BASE + storage_offset + slot * FLASH_SECTOR_SIZE;
    std::memcpy(page.data(), reinterpret_cast<const void *>(address), page.size());
    return true;
}

struct FlashWrite {
    uint32_t offset;
    const Page &page;
};

void write_flash(void *arg) {
    const auto &request = *static_cast<FlashWrite *>(arg);
    flash_range_erase(request.offset, FLASH_SECTOR_SIZE);
    flash_range_program(request.offset, request.page.data(), FLASH_PAGE_SIZE);
}

bool replace_flash(void *, unsigned slot, const Page &page) {
    FlashWrite write{storage_offset + slot * FLASH_SECTOR_SIZE, page};
    flash_interrupted_tach = true;
    return flash_safe_execute(write_flash, &write, 100) == PICO_OK;
}
} // namespace

Tick now_ms() { return time_us_64() / 1000; }

void apply_pwm(const Output &output) {
    for (unsigned i = 0; i < fan_count; ++i) {
        const auto level = (pwm_period * (100u - output.pwm[i]) + 50u) / 100;
        pwm_set_gpio_level(pwm_pins[i], static_cast<uint16_t>(level));
    }
}

void init_pwm(const Output &initial) {
    pwm_period = static_cast<uint16_t>((clock_get_hz(clk_sys) + pwm_hz / 2) / pwm_hz);
    for (unsigned pin : pwm_pins) {
        auto config = pwm_get_default_config();
        pwm_config_set_wrap(&config, pwm_period - 1);
        pwm_init(pwm_gpio_to_slice_num(pin), &config, false);
    }
    apply_pwm(initial);
    for (unsigned pin : pwm_pins) {
        pwm_set_enabled(pwm_gpio_to_slice_num(pin), true);
        gpio_set_function(pin, GPIO_FUNC_PWM);
    }
}

void start_acquisition(Tick now) {
    adc_init();
    for (unsigned pin : adc_pins)
        adc_gpio_init(pin);
    for (unsigned pin : tach_pins) {
        gpio_init(pin);
        gpio_disable_pulls(pin); // R10/R13 provide 3.3 V pull-up.
    }
    gpio_set_irq_enabled_with_callback(tach_pins[0], GPIO_IRQ_EDGE_FALL, true, tach_irq);
    gpio_set_irq_enabled(tach_pins[1], GPIO_IRQ_EDGE_FALL, true);

    i2c_init(i2c1, bus_hz);
    gpio_set_function(sda, GPIO_FUNC_I2C);
    gpio_set_function(scl, GPIO_FUNC_I2C);
    gpio_disable_pulls(sda);
    gpio_disable_pulls(scl);
    ina_acquisition.next = now;
    next_adc = now + 5;
    tach_start = now;
}

bool acquire(Tick now, Snapshot &latest) {
    bool updated = ina_acquisition.step(now, latest.ina);
    if (now >= next_adc) {
        for (unsigned i = 0; i < fan_count; ++i) {
            adc_select_input(adc_pins[i] - 26);
            uint16_t count{};
            // Discard the first read after changing channels. Both reads are bounded.
            const bool ready = read_adc(count) && read_adc(count);
            latest.adc[i] = {now, ready, count};
        }
        next_adc = now + sample_interval;
        updated = true;
    }
    if (flash_interrupted_tach || now - tach_start >= tach_interval) {
        const auto irq = save_and_disable_interrupts();
        for (unsigned i = 0; i < fan_count; ++i) {
            const auto total = pulse_counts[i];
            latest.tach[i] = flash_interrupted_tach
                                 ? Tach{}
                                 : Tach{now, now - tach_start, pulse_times[i],
                                        pulse_delta(total, previous_counts[i]), ever_pulsed[i]};
            previous_counts[i] = total;
            if (flash_interrupted_tach) ever_pulsed[i] = false;
        }
        restore_interrupts(irq);
        tach_start = now;
        flash_interrupted_tach = false;
        updated = true;
    }
    return updated;
}

StorageBackend flash_storage() { return {nullptr, read_flash, replace_flash}; }
} // namespace cmfc::pico
