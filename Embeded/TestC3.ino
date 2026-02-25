#include <config/config.h>
#include <Arduino.h>
#include <logger/logger.h>
#include <SPWiFi.hpp>
#include <udp/udp.h>
#include <utils/Utils.h>
#include <SettingsAsync.h>
#include <GyverDBFile.h>
#include <led/led.h>

#ifdef USE_MOTOR
#include <motor/Motor.h>
#endif

#ifdef USE_SPEEDOMETER
#include <speedometer/Speedometer.h>
    #ifdef USE_MOTOR
    #include <pid\Pid.h>
    #endif
#endif

#ifdef USE_SHEME_MARKER
#include <schememarker/schememarker.h>
#endif

#ifdef USE_SWITCH
#include <switch/Switch.h>
#endif

#ifdef USE_SEMAPHORE
#include <semaphore/Semaphore.h>
#endif

#include <status/Status.h>

static const char TAG[] PROGMEM = "MAIN";

#ifdef USE_MOTOR
    static Motor *motor;
#endif

#ifdef USE_SPEEDOMETER
    static Speedometer *speedometer;

    #ifdef USE_MOTOR
        static PID *pid;
        static PIDTask *pidTask;

        static float IRAM_ATTR pidSourceCallback(void *args) noexcept {
            return speedometer->getSpeed();
        }

        static void IRAM_ATTR pidDestinationCallback(void *args, float value) noexcept {
            motor->setValueSmoothly(value);
        }
    #endif
#endif

#ifdef USE_SHEME_MARKER
    static SchemeMarker *schemeMarker = nullptr;
#endif

#ifdef USE_SWITCH
    static Switch *r_switch;
#endif

#ifdef USE_SEMAPHORE
    static Semaphores *semaphores;
#endif

#ifdef WITH_LEDS
    static LedTask ledTask;
    static Led *leds[2];
#endif

static Status *status = nullptr;

static SPWiFi wifi;

static GyverDBFile db(&LittleFS, "/data.db");

static SettingsAsync settingsAsync("Настройки", &db);
DB_KEYS(
    UI,
    wifi_ssid,
    wifi_pass,
    ap_ssid,
    ap_pass,
    hostname,
    apply,
    #ifdef USE_MOTOR
        motor_pin_forward,
        motor_pin_backward,
        motor_power,
        motor_smooth_ms,
        motor_smooth_steps,
        motor_pwm_value,
    #endif

    #ifdef USE_SPEEDOMETER
        speedometer_pin,
        speedometer_mm_per_turn,
        speed,
        s_rps,
        s_rotations,
        #ifdef USE_MOTOR
            pid_Kp,
            pid_Kd,
            pid_Ki,
            pid_min,
            pid_max,
        #endif
    #endif

    #ifdef USE_SHEME_MARKER
        schememarker_pin,
        schememarker_time,
        schememarker_reversed,
    #endif

    #ifdef USE_SWITCH
        switch_pin,
        switch_release_state,
        switch_smooth_ms,
        switch_smooth_steps,
        switch_max_value,
        switch_stab_value,
        switch_state,
    #endif

    #ifdef USE_INTERNAL_TEMP
        internal_temp,
    #endif

    #ifdef USE_SEMAPHORE
        semaphore_pin,
    #endif

    #ifdef USE_SEMAPHORE
        semaphore_led11,
        semaphore_switch11,
        semaphore_led12,
        semaphore_switch12,
        semaphore_led13,
        semaphore_switch13,

        #ifdef I_M_SWITCH_WITH_SEMAPHORES
            semaphore_led21,
            semaphore_led22,
            semaphore_led23,

            semaphore_led31,
            semaphore_led32,
            semaphore_led33,

            semaphore_led41,
            semaphore_led42,
            semaphore_led43,

            semaphore_led51,
            semaphore_led52,
            semaphore_led53,
        #endif
    #endif 

    #ifdef WITH_LEDS
        led_left_pin,
        led_right_pin,
        led_left,
        led_right,
        led_left_switch,
        led_right_switch,
    #endif

    current_time,
    use_console,
    handshake_inverval_s,
    rssi,
    ip_adress);

static AsyncUDP asyncUdp;

static IPAddress hostAddress((uint32_t)0);

static String hostName;

static UDPSender udpSenderBroadcast(asyncUdp, [](IPAddress *address, uint16_t *port) noexcept {
    *port = 10037;
    return true;
}, true, UDP_TASK_PRIORITY - 1, 10);

static int16_t IRAM_ATTR sendHandshakeCallback(void *args, uint8_t *buffer, size_t maxLen) {
    const uint64_t currentTimeMs = currentTimeMillis();

    // time is not sync. Ignore
    if (unlikely(currentTimeMs < ZERO_TIME_THRESHOLD_SEC * 1000))
        return 0;

    return (int16_t) BinarySerializer::serialize(buffer, maxLen,
            (uint8_t) PACKET_TYPE_HANDSHAKE,
            (const char*) hostName.c_str(),
            (uint16_t) SCHEME_ITEM_TYPE,
            (uint64_t) currentTimeMs,
            #ifdef I_M_SWITCH_WITH_SEMAPHORES
                (uint8_t) 5
            #else
                (uint8_t) 0
            #endif
            );
}

static uint16_t handshakeTimeout;
static bool IRAM_ATTR addressCallback(IPAddress *address, uint16_t *port) {
    static volatile uint32_t lastSendHandshakeTime = 0;

    const uint32_t timeSec = (uint32_t)(esp_timer_get_time() / 1000000);
    const uint32_t timeDiffSec = timeSec - lastSendHandshakeTime;

    if (unlikely(timeDiffSec >= handshakeTimeout || (uint32_t(hostAddress) == 0 && timeDiffSec >= 1))) {
        lastSendHandshakeTime = timeSec;
        udpSenderBroadcast.send(sendHandshakeCallback, nullptr);
    }

    *address = hostAddress;
    *port = 10037;

    return uint32_t(hostAddress) != 0;
}

static UDPSender udpSender(asyncUdp, addressCallback);

static UdpLogger udpLogger(udpSender);

static char *getTimeAsString(const uint64_t ms, char * const dst) noexcept {
    if (ms == 0)
        *dst = '\0';
    else {
        time_t time = divBy1000(ms);
        static struct tm t;
        t = *localtime(&time);
        sprintf(dst, "%.2d.%.2d.%.4d %.2d:%.2d:%.2d,%.3d", t.tm_mday, t.tm_mon + 1, t.tm_year + 1900, t.tm_hour, t.tm_min, t.tm_sec, (int)(ms % 1000));
    }

    return dst;
}

static char *getCurrentTimeAsString(char *dst) noexcept {
    static struct tm t;
    struct timeval time;

    gettimeofday(&time, NULL);
    t = *localtime(&time.tv_sec);
    sprintf(dst, "%.2d.%.2d.%.4d %.2d:%.2d:%.2d,%.3d", t.tm_mday, t.tm_mon + 1, t.tm_year + 1900, t.tm_hour, t.tm_min, t.tm_sec, (int)divBy1000(time.tv_usec));

    return dst;
}

#ifdef USE_INTERNAL_TEMP
    inline float __attribute__((__always_inline__)) getInternalTemp() noexcept {
        return likely(status) ? status->getInternalTemp() : 0.0f;
    }
#endif

#ifdef USE_MOTOR
    static float power = 0.0;
    static float pwm_value = 0.0;
    #ifdef USE_SPEEDOMETER
        static float speed_value = 0.0;
    #endif
#endif

#ifdef USE_SWITCH
    static bool switchState;
#endif

#ifdef USE_SEMAPHORE
    static bool semaphoreLed11 = false;
    static bool semaphoreLed12 = false;
    static bool semaphoreLed13 = false;
#endif

#ifdef WITH_LEDS
    static bool ledLeftVal = false;
    static bool ledRightVal = false;
#endif

static char timeStr[128];
static char timeStr1[128];
void buildUI(sets::Builder& b) {
    static Text EMPTY_TEXT = "";

    {   
        b.beginMenu("Настройки");

        b.beginGroup("Подключение к точке доступа");
        b.Input(UI::wifi_ssid, "SSID");
        b.Pass(UI::wifi_pass, "Пароль");
        b.endGroup();

        b.beginGroup("Точка доступа");
        b.Input(UI::ap_ssid, "SSID");
        b.Pass(UI::ap_pass, "Пароль");
        b.endGroup();

        b.Input(UI::hostname, "Имя устройства");
        b.Number(UI::handshake_inverval_s, "Handshake, сек", nullptr, 1, 60);

        #ifdef USE_MOTOR
            b.beginGroup("Мотор");
            b.Number(UI::motor_pin_forward, "Пин \"Вперед\"", nullptr, 0, 21);
            b.Number(UI::motor_pin_backward, "Пин \"Назад\"", nullptr, 0, 21);
            b.Number(UI::motor_smooth_ms, "Время установки мощности, мс", nullptr, 0, 10000);
            b.Number(UI::motor_smooth_steps, "Количество шагов установки", nullptr, 1, 100);
            b.endGroup();
        #endif

        #ifdef USE_SPEEDOMETER
            b.beginGroup("Спидометр");
            b.Number(UI::speedometer_pin, "Пин", nullptr, 0, 21);
            b.Number(UI::speedometer_mm_per_turn, "Длина окружности оборота, мм", nullptr, 0.0, 200.0);
            b.endGroup();

            #ifdef USE_MOTOR
                b.beginGroup("ПИД");
                b.Number(UI::pid_Kp, "Kp", nullptr, 0.0, 1000000.0);
                b.Number(UI::pid_Kd, "Kd", nullptr, 0.0, 1000000.0);
                b.Number(UI::pid_Ki, "Ki", nullptr, 0.0, 1000000.0);
                b.Number(UI::pid_min, "Мин", nullptr, 0.0, 100.0);
                b.Number(UI::pid_max, "Макс", nullptr, 0.0, 100.0);
                b.endGroup();
            #endif
        #endif

        #ifdef USE_SHEME_MARKER
            b.beginGroup("Маркер схемы");
            b.Number(UI::schememarker_pin, "Пин", nullptr, 0, 21);
            b.Switch(UI::schememarker_reversed, "Реверсивный датчик");
            b.endGroup();
        #endif

        #ifdef USE_SWITCH
            b.beginGroup("Стрелка");
            b.Number(UI::switch_pin, "Пин", nullptr, 0, 21);
            b.Switch(UI::switch_release_state, "Состояние выключено");
            b.Number(UI::switch_smooth_ms, "Время установки мощности, мс", nullptr, 0, 2000);
            b.Number(UI::switch_smooth_steps, "Количество шагов установки", nullptr, 1, 100);
            b.Number(UI::switch_max_value, "Максимальное значение", nullptr, 1, 1023);
            b.Number(UI::switch_stab_value, "Значение удержания", nullptr, 1, 1023);
            b.endGroup();
        #endif

        #ifdef USE_SEMAPHORE
            b.beginGroup("Семафор");
            b.Number(UI::semaphore_pin, "Пин", nullptr, 0, 21);
            b.endGroup();
        #endif 

        #ifdef WITH_LEDS
            b.beginGroup("Свет");
            b.Number(UI::led_left_pin, "Левая фара (пин)", nullptr, 0, 21);
            b.Number(UI::led_right_pin, "Правая фара (пин)", nullptr, 0, 21);
            b.endGroup();
        #endif

        b.beginGroup("Разное");
        b.Switch(UI::use_console, "Использовать консоль");
        b.endGroup();

        b.beginButtons();
        if (b.Button(UI::apply, "Сохранить и перезагрузить")) {
            db.update();

            delay(1000);

            ESP.restart();
        }
        b.endButtons();

        b.endMenu();

        b.beginGroup("Состояние");
        b.Label(UI::ip_adress, "IP адрес", wifi.getIP().toString());

        #ifdef USE_MOTOR
            if (b.Number(UI::motor_power, "Мощность, %", &power, -100.0, 100.0)) {
                SP_LOGI(TAG, "Set power=%f", power);
    
                motor->setPowerSmoothly(power);
            }

            if (b.Number(UI::motor_pwm_value, "PWM", &pwm_value, -1023.0, 1023.0)) {
                SP_LOGI(TAG, "Set pwm=%d", (int) pwm_value);
                
                motor->setPwmValue((int16_t) pwm_value);
            }

            // b.PlotRunning(UI::power_plot, "key1;key2;key3;key4");
        #endif

        #ifdef USE_SPEEDOMETER
            #ifdef USE_MOTOR
                if (b.Number(UI::speed, "Скорость, мм/с", &speed_value, -200.0, 200.0)) {
                    SP_LOGI(TAG, "Set speed=%f", speed_value);
        
                    pid->setTarget(speed_value);
                }
            #else
                b.LabelNum(UI::speed, "Скорость, мм/с", speedometer->getSpeed());
            #endif

            b.LabelNum(UI::s_rps, "RPS", speedometer->getRPS());
            b.LabelNum(UI::s_rotations, "Кол-во оборотов", speedometer->getRotations());
        #endif

        #ifdef USE_SHEME_MARKER
            b.Label(UI::schememarker_time, "Время маркера", getTimeAsString(schemeMarker->getLastTime(), timeStr1));
        #endif

        #ifdef USE_SWITCH
            if (b.Switch(UI::switch_state, "Состояние стрелки", &switchState)) {
                r_switch->setValue(switchState);
            }
        #endif

        #ifdef I_M_SEMAPHORE
            b.beginRow(EMPTY_TEXT);
            b.Label("Семафор");
            SColor color = semaphores->get(0).getValue();
            const bool isYellow = color.getR() && color.getG();

            if (b.Switch(UI::semaphore_switch11, EMPTY_TEXT, &semaphoreLed11)) {
                SP_LOGI(TAG, "Change semaphore: %d", (int) semaphoreLed11);
                semaphores->get(0).setValue(SColor(255, 0, 0));
            }
            b.LED(UI::semaphore_led11, EMPTY_TEXT, !isYellow && color.getR(), sets::Colors::Gray, sets::Colors::Red);

            b.LED(UI::semaphore_led12, EMPTY_TEXT, isYellow, sets::Colors::Gray, sets::Colors::Yellow);
            b.LED(UI::semaphore_led13, EMPTY_TEXT, !isYellow && color.getG(), sets::Colors::Gray, sets::Colors::Green);
            b.endRow();
        #else

        #ifdef WITH_LEDS
            b.beginRow(EMPTY_TEXT);
            b.Label("Левая фара");
            if (b.Switch(UI::led_left_switch, EMPTY_TEXT, &ledLeftVal))
                leds[0]->setValue((uint8_t) (ledLeftVal ? 0xFF : 0));
            b.LED(UI::led_left, EMPTY_TEXT, leds[0]->getValue() != 0, 0x808080, 0xE7E700);
            b.endRow();

            b.beginRow(EMPTY_TEXT);
            b.Label("Правая фара");
            if (b.Switch(UI::led_right_switch, EMPTY_TEXT, &ledRightVal))
                leds[1]->setValue((uint8_t) (ledRightVal ? 0xFF : 0));
            b.LED(UI::led_right, EMPTY_TEXT, leds[1]->getValue() != 0, 0x808080, 0xE7E700);
            b.endRow();
        #endif

        #ifdef I_M_SWITCH_WITH_SEMAPHORES
            b.beginRow(EMPTY_TEXT);
            b.Label("Семафор 1");
            Semaphore &semaphore = semaphores->get(0);
            SColor color = semaphore.getValue();
            bool isYellow = color.getR() && color.getG();
            if (b.Switch(UI::semaphore_switch11, EMPTY_TEXT, &semaphoreLed11))
                semaphore.setValue(semaphoreLed11 ? SColor::RED : SColor::BLACK);
            b.LED(UI::semaphore_led11, EMPTY_TEXT, !isYellow && color.getR(), sets::Colors::Gray, sets::Colors::Red);
            if (b.Switch(UI::semaphore_switch12, EMPTY_TEXT, &semaphoreLed12))
                semaphore.setValue(semaphoreLed12 ? SColor::YELLOW : SColor::BLACK);
            b.LED(UI::semaphore_led12, EMPTY_TEXT, isYellow, sets::Colors::Gray, sets::Colors::Yellow);
            if (b.Switch(UI::semaphore_switch13, EMPTY_TEXT, &semaphoreLed13))
                semaphore.setValue(semaphoreLed13 ? SColor::GREEN : SColor::BLACK);
            b.LED(UI::semaphore_led13, EMPTY_TEXT, !isYellow && color.getG(), sets::Colors::Gray, sets::Colors::Green);
            b.endRow();

            b.beginRow(EMPTY_TEXT);
            b.Label("Семафор 2");
            color = semaphores->get(1).getValue();
            isYellow = color.getR() && color.getG();
            b.LED(UI::semaphore_led21, EMPTY_TEXT, !isYellow && color.getR(), sets::Colors::Gray, sets::Colors::Red);
            b.LED(UI::semaphore_led22, EMPTY_TEXT, isYellow, sets::Colors::Gray, sets::Colors::Yellow);
            b.LED(UI::semaphore_led23, EMPTY_TEXT, !isYellow && color.getG(), sets::Colors::Gray, sets::Colors::Green);
            b.endRow();

            b.beginRow(EMPTY_TEXT);
            b.Label("Семафор 3");
            color = semaphores->get(2).getValue();
            isYellow = color.getR() && color.getG();
            b.LED(UI::semaphore_led31, EMPTY_TEXT, !isYellow && color.getR(), sets::Colors::Gray, sets::Colors::Red);
            b.LED(UI::semaphore_led32, EMPTY_TEXT, isYellow, sets::Colors::Gray, sets::Colors::Yellow);
            b.LED(UI::semaphore_led33, EMPTY_TEXT, !isYellow && color.getG(), sets::Colors::Gray, sets::Colors::Green);
            b.endRow();

            b.beginRow(EMPTY_TEXT);
            b.Label("Семафор 4");
            color = semaphores->get(3).getValue();
            isYellow = color.getR() && color.getG();
            b.LED(UI::semaphore_led41, EMPTY_TEXT, !isYellow && color.getR(), sets::Colors::Gray, sets::Colors::Red);
            b.LED(UI::semaphore_led42, EMPTY_TEXT, isYellow, sets::Colors::Gray, sets::Colors::Yellow);
            b.LED(UI::semaphore_led43, EMPTY_TEXT, !isYellow && color.getG(), sets::Colors::Gray, sets::Colors::Green);
            b.endRow();

            b.beginRow(EMPTY_TEXT);
            b.Label("Семафор 5");
            color = semaphores->get(4).getValue();
            isYellow = color.getR() && color.getG();
            b.LED(UI::semaphore_led51, EMPTY_TEXT, !isYellow && color.getR(), sets::Colors::Gray, sets::Colors::Red);
            b.LED(UI::semaphore_led52, EMPTY_TEXT, isYellow, sets::Colors::Gray, sets::Colors::Yellow);
            b.LED(UI::semaphore_led53, EMPTY_TEXT, !isYellow && color.getG(), sets::Colors::Gray, sets::Colors::Green);
            b.endRow();
        #endif
        #endif

        b.LabelNum(UI::rssi, "RSSI", wifi.getRSSI());

        #ifdef USE_INTERNAL_TEMP
            b.LabelFloat(UI::internal_temp, "Температура чипа, C", getInternalTemp(), 1);
        #endif

        b.Label(UI::current_time, "Время", getCurrentTimeAsString(timeStr));

        b.endGroup();
    }
} 

void updateUI(sets::Updater& upd) {
    upd.update(UI::ip_adress, wifi.getIP().toString());
    
    #ifdef USE_MOTOR
        upd.update(UI::motor_power, power = motor->getPower());

        // static float pw[4];
        // pw[0] = 10;
        // pw[1] = 7;
        // pw[2] = 8;
        // pw[3] = 2;
        // upd.updatePlot(UI::power_plot, pw);
    #endif

    #ifdef USE_SPEEDOMETER
        upd.update(UI::speed, 
            #ifdef USE_MOTOR 
                speed_value = motor->getDirection() * 
            #endif 
            speedometer->getSpeed()
        );

        upd.update(UI::s_rps, speedometer->getRPS());
        upd.update(UI::s_rotations, speedometer->getRotations());
    #endif

    upd.update(UI::rssi, wifi.getRSSI());

    #ifdef USE_INTERNAL_TEMP
        upd.update(UI::internal_temp, getInternalTemp());
    #endif

    upd.update(UI::current_time, getCurrentTimeAsString(timeStr));

    #ifdef USE_SHEME_MARKER
        upd.update(UI::schememarker_time, getTimeAsString(schemeMarker->getLastTime(), timeStr1));
    #endif

    #ifdef USE_SWITCH
        upd.update(UI::switch_state, switchState = r_switch->getValue());
    #endif

    #ifdef WITH_LEDS
        ledLeftVal = leds[0]->getValue() != 0;
        upd.update(UI::led_left, ledLeftVal);
        upd.update(UI::led_left_switch, ledLeftVal);

        ledRightVal = leds[1]->getValue() != 0;
        upd.update(UI::led_right, ledRightVal);
        upd.update(UI::led_right_switch, ledRightVal);
    #endif

    #ifdef USE_SEMAPHORE
        SColor color = semaphores->get(0).getValue();
        bool isYellow = color.getR() && color.getG();
        
        semaphoreLed11 = !isYellow && color.getR();
        upd.update(UI::semaphore_led11, semaphoreLed11);
        upd.update(UI::semaphore_switch11, semaphoreLed11);

        semaphoreLed12 = isYellow;
        upd.update(UI::semaphore_led12, semaphoreLed12);
        upd.update(UI::semaphore_switch12, semaphoreLed12);

        semaphoreLed13 = !isYellow && color.getG();
        upd.update(UI::semaphore_led13, semaphoreLed13);
        upd.update(UI::semaphore_switch13, semaphoreLed13);

        #ifdef I_M_SWITCH_WITH_SEMAPHORES
            color = semaphores->get(1).getValue();
            isYellow = color.getR() && color.getG();
            upd.update(UI::semaphore_led21, !isYellow && color.getR());
            upd.update(UI::semaphore_led22, isYellow);
            upd.update(UI::semaphore_led23, !isYellow && color.getG());

            color = semaphores->get(2).getValue();
            isYellow = color.getR() && color.getG();
            upd.update(UI::semaphore_led31, !isYellow && color.getR());
            upd.update(UI::semaphore_led32, isYellow);
            upd.update(UI::semaphore_led33, !isYellow && color.getG());

            color = semaphores->get(3).getValue();
            isYellow = color.getR() && color.getG();
            upd.update(UI::semaphore_led41, !isYellow && color.getR());
            upd.update(UI::semaphore_led42, isYellow);
            upd.update(UI::semaphore_led43, !isYellow && color.getG());

            color = semaphores->get(4).getValue();
            isYellow = color.getR() && color.getG();
            upd.update(UI::semaphore_led41, !isYellow && color.getR());
            upd.update(UI::semaphore_led42, isYellow);
            upd.update(UI::semaphore_led43, !isYellow && color.getG());
        #endif
    #endif
}

static void IRAM_ATTR parseCommand(IPAddress& remoteIP, uint8_t messageType, uint8_t *buffer, size_t length, void *args) noexcept {
    switch(messageType) {
        case PACKET_TYPE_HANDSHAKE_RESPONSE:
            hostAddress = remoteIP;
            if (length > 0)
                SP_LOGW(TAG, "Handshake lenght non zero - %d", (int) length);
            break;

        #ifdef USE_MOTOR
            case PACKET_TYPE_POWER:
                if (length == 4)
                    motor->setPowerSmoothly((*(int32_t*) buffer) / 1000.0f);
                else
                    SP_LOGW(TAG, "Set power error len=%d", (int) length);
                break;
            
            #ifdef USE_SPEEDOMETER
            case PACKET_TYPE_SPEED:
                if (length == 4)
                    pidTask->setTarget((float) *(int32_t*) buffer);
                else
                    SP_LOGW(TAG, "Set speed error len=%d", (int) length);
                break;
            #endif
        #endif

        #ifdef USE_SWITCH
            case PACKET_TYPE_SWITCH:
                if (length == 1)
                    r_switch->setValue(*(uint8_t*) buffer);
                else
                    SP_LOGW(TAG, "Set switch error len=%d", (int) length);
                break;
        #endif

        #ifdef USE_SEMAPHORE
            case PACKET_TYPE_SEMAPHORE:
                if (length > 2) {
                    const uint16_t count = *(uint16_t*) buffer;
                    uint8_t *buf = buffer + 2;
                    length -= 2;

                    for (uint16_t i = 0; i < count; i++, buf += 6, length -= 6) {
                        if (unlikely(length < 6)) {
                            SP_LOGW(TAG, "Unknown semaphore item length =%d", (int) length);
                            break;
                        }

                        const uint16_t index = *(uint16_t*) buf;

                        if (index < semaphores->count())
                            semaphores->get(index).setValue(*(uint32_t*) (buffer + 2));
                        else
                            SP_LOGW(TAG, "Set semaphore error index=%d", (int) index);
                    }
                }
                else
                    SP_LOGW(TAG, "Set switch error len=%d", (int) length);
                break;
        #endif 

        case PACKET_TYPE_HANDSHAKE:
            break;

        default:
            SP_LOGW(TAG, "Unknown packet type=%d, len=%d", (int) messageType, (int) length);
            break;
    }
}

void onUdpPacket(void *arg, AsyncUDPPacket &packet) {
    parsePacket(packet, parseCommand, arg);
}

static bool USE_CONSOLE;

static void initDB() noexcept {
    LittleFS.begin(true);
    db.begin();

    db.init(UI::wifi_ssid, "SPDC");
    db.init(UI::wifi_pass, "743855341921");

    #ifdef I_M_TRAIN
        db.init(UI::ap_ssid, "train");
        db.init(UI::hostname, "train");
    #endif

    #ifdef I_M_SEMAPHORE
        db.init(UI::ap_ssid, "semaphore");
        db.init(UI::hostname, "semaphore");
    #endif

    #ifdef I_M_SWITCH
        db.init(UI::ap_ssid, "switch");
        db.init(UI::hostname, "switch");
    #endif

    #ifdef I_M_SWITCH_WITH_SEMAPHORES
        db.init(UI::ap_ssid, "switch-sem");
        db.init(UI::hostname, "switch-sem");
    #endif

    db.init(UI::ap_pass, "spider");

    #ifdef USE_MOTOR
        db.init(UI::motor_pin_forward, (uint8_t) 0);
        db.init(UI::motor_pin_backward, (uint8_t) 1);
        db.init(UI::motor_smooth_ms, (uint16_t) 1500);
        db.init(UI::motor_smooth_steps, (uint16_t) 20);
    #endif

    #ifdef USE_SPEEDOMETER
        db.init(UI::speedometer_pin, (uint8_t) 2);
        db.init(UI::speedometer_mm_per_turn, (float) 10.0);
        #ifdef USE_MOTOR
            db.init(UI::pid_Kp, (float) 0.2);
            db.init(UI::pid_Kd, (float) 0.0);
            db.init(UI::pid_Ki, (float) 0.6);
            db.init(UI::pid_min, (float) 410);
            db.init(UI::pid_max, (float) 1023);
        #endif
    #endif

    #ifdef USE_SHEME_MARKER
        db.init(UI::schememarker_pin, (uint8_t) 3);
        db.init(UI::schememarker_reversed, true);
    #endif

    #ifdef USE_SWITCH
        db.init(UI::switch_pin, (uint8_t) 0);
        db.init(UI::switch_release_state, false);
        db.init(UI::switch_smooth_ms, (uint16_t) 200);
        db.init(UI::switch_smooth_steps, (uint16_t) 8);
        db.init(UI::switch_max_value, (uint16_t) 700);
        db.init(UI::switch_stab_value, (uint16_t) 100);
    #endif

    #ifdef USE_SEMAPHORE
        db.init(UI::semaphore_pin, (uint8_t)
            #ifdef USE_SWITCH
                1
            #else
                0
            #endif            
        );
    #endif

    #ifdef WITH_LEDS
        db.init(UI::led_left_pin, (uint8_t) 2);
        db.init(UI::led_right_pin, (uint8_t) 4);
    #endif

    db.init(UI::use_console, true);
    db.init(UI::handshake_inverval_s, (uint16_t) 30);
}

static std::atomic_bool runOnce = false;

void setup() {
    vTaskPrioritySet(NULL, NORMAL_TASK_PRIORITY);

    initDB();

    handshakeTimeout = db[UI::handshake_inverval_s].toInt16();
    
    if ((USE_CONSOLE = db[UI::use_console].toBool())) {
        Serial.begin(115200);
        currentLogger = new Logger(2, &udpLogger, new ConsoleLogger());
    } else 
        currentLogger = new Logger(1, &udpLogger);

    #ifdef USE_MOTOR
        motor = new Motor((uint8_t)db[UI::motor_pin_forward].toInt16(), (uint8_t)db[UI::motor_pin_backward].toInt16(),
                        db[UI::motor_smooth_ms].toInt16(), db[UI::motor_smooth_steps].toInt16());
        motor->begin();
    #endif

    #ifdef USE_SPEEDOMETER
        speedometer = new Speedometer((uint8_t)db[UI::speedometer_pin].toInt16(), db[UI::speedometer_mm_per_turn].toFloat());
        speedometer->begin();

        #ifdef USE_MOTOR
            pid = new PID(db[UI::pid_Kp].toFloat(), db[UI::pid_Kd].toFloat(), db[UI::pid_Ki].toFloat(),
                            db[UI::pid_min].toFloat(), db[UI::pid_max].toFloat());

            pidTask = new PIDTask(*pid, pidSourceCallback, nullptr, pidDestinationCallback, nullptr);
        #endif
    #endif

    #ifdef USE_SHEME_MARKER
        schemeMarker = new SchemeMarker((uint8_t)db[UI::schememarker_pin].toInt16(), db[UI::schememarker_reversed].toBool());
        schemeMarker->begin();
    #endif

    #ifdef USE_SWITCH
        r_switch = new SimpleSwitch(db[UI::switch_release_state].toBool(), (uint8_t)db[UI::switch_pin].toInt16(),
                        db[UI::switch_smooth_ms].toInt16(), db[UI::switch_smooth_steps].toInt16(),
                        db[UI::switch_max_value].toInt16(), db[UI::switch_stab_value].toInt16());
        r_switch->begin();
    #endif

    #ifdef I_M_SEMAPHORE
        semaphores = new FlSemaphores<1>((uint8_t)db[UI::semaphore_pin].toInt16());
    #endif 

    #ifdef I_M_SWITCH_WITH_SEMAPHORES
        semaphores = new FlRGBSemaphores((uint8_t)db[UI::semaphore_pin].toInt16(), 5);
    #endif

    #ifdef USE_SEMAPHORE
        semaphores->begin();
    #endif

    #ifdef WITH_LEDS
        leds[0] = new SmoothlyLed(*new PwmLed((uint8_t)db[UI::led_left_pin].toInt16()), ledTask);
        leds[1] = new SmoothlyLed(*new PwmLed((uint8_t)db[UI::led_right_pin].toInt16()), ledTask);

        leds[0]->begin();
        leds[1]->begin();
    #endif

    status = new Status(
        #ifdef USE_MOTOR
            *motor,
        #endif

        #ifdef USE_SPEEDOMETER
            *speedometer,
        #endif

        #ifdef USE_SHEME_MARKER
            *schemeMarker,
        #endif

        #ifdef USE_SWITCH
            *r_switch,
        #endif

        #ifdef USE_SEMAPHORE
            *semaphores,
        #endif

        #ifdef WITH_LEDS
            leds, 2,
        #endif

        udpSender);

    asyncUdp.onPacket(onUdpPacket);

    settingsAsync.onBuild(buildUI);
    settingsAsync.onUpdate(updateUI);

    wifi.begin(
        db[UI::wifi_ssid].toString(), db[UI::wifi_pass].toString(),
        db[UI::ap_ssid].toString(), db[UI::ap_pass].toString(),
        hostName = db[UI::hostname].toString(),
        [](IPAddress &ipAdress) noexcept {
            if (!runOnce.exchange(true)) {
                configTime(3 * 3600, 0, "europe.pool.ntp.org", "pool.ntp.org");
                asyncUdp.listen(10037);

                status->begin();
                settingsAsync.begin();

                udpSenderBroadcast.send(sendHandshakeCallback, nullptr);
                SP_LOGI(TAG, "Connect %s", ipAdress.toString().c_str());
            } else 
                SP_LOGI(TAG, "Reconnect %s", ipAdress.toString().c_str());
        },
        []() noexcept {
            SP_LOGI(TAG, "Disconnect");
        }
    );
}

#define LOOP_DELAY_MS 250

void loop() {
    static volatile uint32_t prevTimeMs = 0;
    const uint32_t currentTimeMs = currentMillis();

    settingsAsync.tick();
    db.tick();

    if (unlikely(USE_CONSOLE)) {
        if ((currentTimeMs - prevTimeMs) >= 5000) {
            prevTimeMs = currentTimeMs;
            SP_LOGI(TAG, "HEARTBEAT");
        }
    }

    const int32_t delayMs = LOOP_DELAY_MS - (((int32_t)currentMillis()) - currentTimeMs);

    if (unlikely(delayMs > 0))
        delay(delayMs);
}
