#pragma once

#include "runtime.hpp"

#include <cstddef>
#include <string_view>

namespace cmfc {
enum class Format { Raw, Physical, Full };

class Protocol {
  public:
    static constexpr size_t receive_limit = 3072;
    static constexpr size_t transmit_limit = 8192;
    explicit Protocol(Runtime &state) : runtime(state) {}

    void connection(bool connected);
    void tick(Tick now, bool allow_stream = true);
    bool feed(char byte, Tick now);
    std::string_view pending() const;
    void consume(size_t count);
    void output(const char *format, ...) __attribute__((format(printf, 2, 3)));

  private:
    Runtime &runtime;
    std::array<char, receive_limit + 8> rx{};
    alignas(16) std::array<char, receive_limit * 12 + 256> arena{};
    std::array<char, transmit_limit> tx{};
    size_t rx_size{}, tx_size{}, sent{};
    bool discarding{}, formatting_error{}, stream{};
    Tick last_byte{}, last_stream{}, interval_ms = 1000;
    Format stream_format = Format::Full;
    uint32_t sequence{}, dropped{};

    void dispatch(Tick now);
    void begin();
    void finish();
    void error(int64_t id, const char *code, bool applied = false);
    void status(Tick now);
    void sensors(Tick now, Format format);
};
} // namespace cmfc
