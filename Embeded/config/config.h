#ifndef __CONFIG_H
#define __CONFIG_H

#define USE_INTERNAL_TEMP

// #define I_M_TRAIN
// #define WITH_LEDS

// #define I_M_SEMAPHORE
#define I_M_SWITCH_WITH_SEMAPHORES
// #define I_M_SWITCH

#define PACKET_TYPE_STATUS 1

#ifdef I_M_TRAIN
    #define USE_MOTOR
    #define USE_SPEEDOMETER
    #define USE_SHEME_MARKER

    #ifdef WITH_LEDS
        #define USE_LEDS
        #define MAX_LEDS_IN_TASK 2
        #define SCHEME_ITEM_TYPE 4
    #else
        #define SCHEME_ITEM_TYPE 0
    #endif

    #define PACKET_TYPE_POWER 4
    #define PACKET_TYPE_SPEED 5
#endif

#ifdef I_M_SWITCH
    #define USE_SWITCH
    #define SCHEME_ITEM_TYPE 1
    #define PACKET_TYPE_SWITCH 6
#endif

#ifdef I_M_SEMAPHORE
    #define USE_LEDS
    #define USE_SEMAPHORE
    #define SCHEME_ITEM_TYPE 2
    #define MAX_LEDS_IN_TASK 9
    #define PACKET_TYPE_SEMAPHORE 7
#endif

#ifdef I_M_SWITCH_WITH_SEMAPHORES
    #define USE_LEDS
    #define USE_SWITCH
    #define USE_SEMAPHORE
    #define SCHEME_ITEM_TYPE 3
    #define MAX_LEDS_IN_TASK 45
    #define PACKET_TYPE_SWITCH 6
    #define PACKET_TYPE_SEMAPHORE 7
#endif

#endif // __CONFIG_H
