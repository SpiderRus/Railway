#include <semaphore/Semaphore.h>

#ifdef USE_SEMAPHORE

const SColor SColor::RED(0x0000FF);
const SColor SColor::GREEN(0x00FF00);
const SColor SColor::YELLOW(0x00FFFF);
const SColor SColor::BLACK(0x000000);

void IRAM_ATTR SemaphoreBase::setValueInternal(const SColor& color) noexcept {
    red.setValue(color.getR());
    green.setValue(color.getG());
    blue.setValue(color.getB());
}

void IRAM_ATTR SemaphoreRGBBase::setValueInternal(const SColor& color) noexcept {
    if (color == SColor::BLACK) {
        green_red.setValue((uint8_t) 0);
        green_green.setValue((uint8_t) 0);
        green_blue.setValue((uint8_t) 0);

        // green_red.setValue((uint8_t) 0);
        // green_green.setValue((uint8_t) 0);
        // green_blue.setValue((uint8_t) 0);

        // yellow_red.setValue((uint8_t) 0);
        // yellow_green.setValue((uint8_t) 0);
        // yellow_blue.setValue((uint8_t) 0);

        // red_red.setValue((uint8_t) 0);
        // red_green.setValue((uint8_t) 0);
        // red_blue.setValue((uint8_t) 0);
    } else if (color == SColor::RED) {
        green_red.setValue((uint8_t) 0xFF);
        green_green.setValue((uint8_t) 0);
        green_blue.setValue((uint8_t) 0);

        // green_red.setValue((uint8_t) 0);
        // green_green.setValue((uint8_t) 0);
        // green_blue.setValue((uint8_t) 0);

        // yellow_red.setValue((uint8_t) 0);
        // yellow_green.setValue((uint8_t) 0);
        // yellow_blue.setValue((uint8_t) 0);

        // red_red.setValue((uint8_t) 0xFF);
        // red_green.setValue((uint8_t) 0);
        // red_blue.setValue((uint8_t) 0);
    } else if (color == SColor::GREEN) {
        green_red.setValue((uint8_t) 0);
        green_green.setValue((uint8_t) 0xFF);
        green_blue.setValue((uint8_t) 0);

        // green_red.setValue((uint8_t) 0);
        // green_green.setValue((uint8_t) 0xFF);
        // green_blue.setValue((uint8_t) 0);

        // yellow_red.setValue((uint8_t) 0);
        // yellow_green.setValue((uint8_t) 0);
        // yellow_blue.setValue((uint8_t) 0);

        // red_red.setValue((uint8_t) 0);
        // red_green.setValue((uint8_t) 0);
        // red_blue.setValue((uint8_t) 0);
    } else if (color == SColor::YELLOW) {
        green_red.setValue((uint8_t) 0xFF);
        green_green.setValue((uint8_t) 0xC0);
        green_blue.setValue((uint8_t) 0);

        // green_red.setValue((uint8_t) 0);
        // green_green.setValue((uint8_t) 0);
        // green_blue.setValue((uint8_t) 0);

        // yellow_red.setValue((uint8_t) 0xFF);
        // yellow_green.setValue((uint8_t) 0xFF);
        // yellow_blue.setValue((uint8_t) 0);

        // red_red.setValue((uint8_t) 0);
        // red_green.setValue((uint8_t) 0);
        // red_blue.setValue((uint8_t) 0);
    } else {
        green_red.setValue((uint8_t) 0xFF);
        green_green.setValue((uint8_t) 0xFF);
        green_blue.setValue((uint8_t) 0xFF);

        // green_red.setValue((uint8_t) 0);
        // green_green.setValue((uint8_t) 0);
        // green_blue.setValue((uint8_t) 0);

        // yellow_red.setValue(color.getR());
        // yellow_green.setValue(color.getG());
        // yellow_blue.setValue(color.getB());

        // red_red.setValue((uint8_t) 0);
        // red_green.setValue((uint8_t) 0);
        // red_blue.setValue((uint8_t) 0);
    }
}

#endif
