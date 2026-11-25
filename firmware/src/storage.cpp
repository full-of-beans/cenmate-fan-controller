#include "storage.hpp"

#include <algorithm>
#include <bit>
#include <limits>

namespace cmfc {
namespace {
constexpr uint32_t magic = 0x3143464d;
constexpr size_t crc_offset = record_size - 4;
constexpr size_t payload_size = 12
#define FIELD(type, key, value, low, high) +4
                                CMFC_FIELDS(FIELD)
#undef FIELD
                                + 12;

static_assert(payload_size <= crc_offset);
static_assert(sizeof(float) == 4 && std::numeric_limits<float>::is_iec559);

void put(Page &page, size_t &offset, uint32_t value) {
    for (unsigned i = 0; i < 4; ++i)
        page[offset++] = static_cast<uint8_t>(value >> (i * 8));
}
uint32_t get(const Page &page, size_t &offset) {
    uint32_t value{};
    for (unsigned i = 0; i < 4; ++i)
        value |= static_cast<uint32_t>(page[offset++]) << (i * 8);
    return value;
}
uint32_t crc32(const Page &page) {
    uint32_t crc = 0xffffffff;
    for (size_t i = 0; i < crc_offset; ++i) {
        crc ^= page[i];
        for (unsigned bit = 0; bit < 8; ++bit)
            crc = (crc >> 1) ^ ((0u - (crc & 1u)) & 0xedb88320u);
    }
    return ~crc;
}
Page encode(const Settings &s, uint32_t generation) {
    Page page{};
    size_t offset{};
    put(page, offset, magic);
    put(page, offset, schema_version);
    put(page, offset, generation);
#define FIELD(type, key, value, low, high) put(page, offset, std::bit_cast<uint32_t>(s.config.key));
    CMFC_FIELDS(FIELD)
#undef FIELD
    put(page, offset, static_cast<uint32_t>(s.operating.mode));
    for (auto pwm : s.operating.pwm)
        put(page, offset, pwm);
    offset = crc_offset;
    put(page, offset, crc32(page));
    return page;
}
bool decode(const Page &page, Settings &s, uint32_t &generation) {
    size_t end = crc_offset;
    if (get(page, end) != crc32(page)) return false;
    size_t offset{};
    if (get(page, offset) != magic || get(page, offset) != schema_version) return false;
    generation = get(page, offset);
#define FIELD(type, key, value, low, high) s.config.key = std::bit_cast<type>(get(page, offset));
    CMFC_FIELDS(FIELD)
#undef FIELD
    const auto mode = get(page, offset);
    const auto upper = get(page, offset);
    const auto lower = get(page, offset);
    if (mode > 1 || upper > 100 || lower > 100) return false;
    s.operating = {static_cast<Mode>(mode), {static_cast<uint8_t>(upper), static_cast<uint8_t>(lower)}};
    return valid(s.config);
}
} // namespace

void Storage::load() {
    persisted = {};
    active_slot = -1;
    sequence = 0;
    invalid = false;
    for (unsigned slot = 0; slot < 2; ++slot) {
        Page page{};
        Settings candidate{};
        uint32_t generation{};
        if (!io.read(io.context, slot, page)) {
            invalid = true;
            continue;
        }
        if (!decode(page, candidate, generation)) {
            invalid |= !std::all_of(page.begin(), page.end(), [](uint8_t value) { return value == 0xff; });
            continue;
        }
        const uint32_t distance = generation - sequence;
        if (active_slot < 0 || (distance && distance < 0x80000000u)) {
            active_slot = static_cast<int>(slot);
            persisted = candidate;
            sequence = generation;
        }
    }
}

bool Storage::save(const Settings &settings) {
    if (!valid(settings.config) || !valid(settings.operating)) return false;
    if (valid_record() && settings == persisted) return true;
    const unsigned target = active_slot == 0 ? 1 : 0;
    const auto page = encode(settings, sequence + 1);
    Page verified{};
    if (!io.replace(io.context, target, page) || !io.read(io.context, target, verified) || verified != page)
        return false;
    persisted = settings;
    ++sequence;
    active_slot = static_cast<int>(target);
    return true;
}
} // namespace cmfc
