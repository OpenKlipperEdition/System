#ifndef __IHAL_GPIO_CONFIG_H__
#define __IHAL_GPIO_CONFIG_H__

#if defined X1600_HALLEY6
    #define GROUP_OUT           GPIOA
    #define PIN_OUT             8
    #define GROUP_Interrupt     GPIOC
    #define PIN_Interrupt       31

#elif defined X2000_HALLEY5
    #define GROUP_OUT           GPIOB
    #define PIN_OUT             4
    #define GROUP_Interrupt     GPIOE
    #define PIN_Interrupt       31

#elif defined X2500_HIPPO
    #define GROUP_OUT           GPIOD
    #define PIN_OUT             0
    #define GROUP_Interrupt     GPIOC
    #define PIN_Interrupt       1

#elif defined X2600_HALLEY
    #define GROUP_OUT           GPIOA
    #define PIN_OUT             6
    #define GROUP_Interrupt     GPIOD
    #define PIN_Interrupt       15

#elif defined X2600E_HALLEY
    #define GROUP_OUT           GPIOA
    #define PIN_OUT             6
    #define GROUP_Interrupt     GPIOD
    #define PIN_Interrupt       15

#elif defined X2670_HALLEY
    #define GROUP_OUT           GPIOA
    #define PIN_OUT             6
    #define GROUP_Interrupt     GPIOD
    #define PIN_Interrupt       15

#elif defined X2670M_HARE
    #define GROUP_OUT           GPIOA
    #define PIN_OUT             6
    #define GROUP_Interrupt     GPIOD
    #define PIN_Interrupt       15

#elif defined X2600_HALLEY7
    #define GROUP_OUT           GPIOA
    #define PIN_OUT             6
    #define GROUP_Interrupt     GPIOD
    #define PIN_Interrupt       15

#elif defined X2670_HARE
    #define GROUP_OUT           GPIOA
    #define PIN_OUT             6
    #define GROUP_Interrupt     GPIOD
    #define PIN_Interrupt       15

#endif

#endif
