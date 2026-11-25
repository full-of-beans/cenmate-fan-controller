#include "pico/unique_id.h"
#include "tusb.h"

namespace {
constexpr tusb_desc_device_t device{
    .bLength = sizeof(tusb_desc_device_t),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = 0x0200,
    .bDeviceClass = TUSB_CLASS_MISC,
    .bDeviceSubClass = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol = MISC_PROTOCOL_IAD,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor = 0x2e8a,
    .idProduct = 0x000a,
    .bcdDevice = 0x0100,
    .iManufacturer = 1,
    .iProduct = 2,
    .iSerialNumber = 3,
    .bNumConfigurations = 1,
};

constexpr uint8_t configuration[]{
    TUD_CONFIG_DESCRIPTOR(1, 2, 0, TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN, 0, 100),
    TUD_CDC_DESCRIPTOR(0, 0, 0x81, 8, 0x02, 0x82, 64),
};
} // namespace

extern "C" const uint8_t *tud_descriptor_device_cb() { return reinterpret_cast<const uint8_t *>(&device); }

extern "C" const uint8_t *tud_descriptor_configuration_cb(uint8_t index) {
    return index == 0 ? configuration : nullptr;
}

extern "C" const uint16_t *tud_descriptor_string_cb(uint8_t index, uint16_t) {
    static uint16_t descriptor[32]{};
    if (index == 0) {
        descriptor[0] = (TUSB_DESC_STRING << 8) | 4;
        descriptor[1] = 0x0409;
        return descriptor;
    }
    char serial[2 * PICO_UNIQUE_BOARD_ID_SIZE_BYTES + 1]{};
    const char *string = nullptr;
    switch (index) {
    case 1:
        string = "Full of Beans";
        break;
    case 2:
        string = "CENMATE CMFC001";
        break;
    case 3:
        pico_get_unique_board_id_string(serial, sizeof(serial));
        string = serial;
        break;
    default:
        return nullptr;
    }
    unsigned length{};
    for (; string[length] && length < 31; ++length)
        descriptor[length + 1] = static_cast<uint8_t>(string[length]);
    descriptor[0] = static_cast<uint16_t>((TUSB_DESC_STRING << 8) | (2 * length + 2));
    return descriptor;
}
