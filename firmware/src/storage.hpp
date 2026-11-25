#pragma once

#include "settings.hpp"

namespace cmfc {
constexpr unsigned record_size = 256;
using Page = std::array<uint8_t, record_size>;

// Each slot must occupy its own independently erasable sector.
struct StorageBackend {
    void *context;
    bool (*read)(void *, unsigned slot, Page &);
    bool (*replace)(void *, unsigned slot, const Page &);
};

class Storage {
  public:
    explicit Storage(StorageBackend backend) : io(backend) {}
    void load();
    bool save(const Settings &settings);
    const Settings &saved() const { return persisted; }
    bool valid_record() const { return active_slot >= 0; }
    bool bad_record() const { return invalid; }
    uint32_t generation() const { return sequence; }

  private:
    StorageBackend io;
    Settings persisted{};
    int active_slot = -1;
    uint32_t sequence{};
    bool invalid{};
};
} // namespace cmfc
