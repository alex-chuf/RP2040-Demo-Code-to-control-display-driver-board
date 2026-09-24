/*
 * Copyright (c) 2026 Shenzhen Sudu Electronics Technology Limited
 * www.sudotronic.com
 * License: MIT
 */



/*
 * EN: Simple demo. Edit the values below, rebuild, and flash the new firmware.
 * 中文：简单演示程序。修改下面的数值后，重新编译并烧录固件即可。
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"

#define REFRESH_SEL_PIN      15   //pick 90Hz(default:0) or 60Hz (1) 
#define TEMP_SENSOR_ADC_PIN  26   //on board NTC sensor ADC pin
#define FAN_CTRL_HIGH_PIN    20
#define FAN_CTRL_LOW_PIN     21
#define BACKLIGHT_PWM_PIN     0   //display brightness control

// EN: Edit these voltages to change when the fan starts and stops.
//     Keep the ON value greater than the OFF value to avoid frequent switching.
// 中文：修改这两个电压值可调整风扇启停条件。
//       开启值应大于关闭值，避免风扇在临界点频繁启停。
#define FAN_ON_THRESHOLD_V   2.10f
#define FAN_OFF_THRESHOLD_V  1.95f

void user_controls_init_before_power_on(void) {
    // EN: Initialize the control pins. Set the refresh level at the end below.
    // 中文：初始化控制引脚。刷新率选择请修改本函数末尾的电平设置。
    gpio_init(REFRESH_SEL_PIN);
    gpio_set_dir(REFRESH_SEL_PIN, GPIO_OUT);
    gpio_put(REFRESH_SEL_PIN, 0); // 0: 90Hz； 1: 60Hz

    gpio_init(FAN_CTRL_HIGH_PIN);
    gpio_set_dir(FAN_CTRL_HIGH_PIN, GPIO_OUT);
    gpio_put(FAN_CTRL_HIGH_PIN, 0);

    gpio_init(FAN_CTRL_LOW_PIN);
    gpio_set_dir(FAN_CTRL_LOW_PIN, GPIO_OUT);
    gpio_put(FAN_CTRL_LOW_PIN, 0);

    // EN: Read the NTC through GPIO26, which is ADC channel 0.
    // 中文：通过 GPIO26 读取 NTC，该引脚对应 ADC 通道 0。
    adc_init();
    adc_gpio_init(TEMP_SENSOR_ADC_PIN);
    adc_select_input(0);

    // EN: GPIO15: 0 = LOW (90 Hz); change 0 to 1 for HIGH (60 Hz).
    //     This selects the startup mode. Verify 60 Hz with your UXE firmware.
    // 中文：GPIO15：0 为低电平（90 Hz）；将下面的 0 改成 1 为高电平（60 Hz）。
    //       此处选择启动模式，60 Hz 请结合所用 UXE 固件实测确认。
    gpio_put(REFRESH_SEL_PIN, 0);
}

void user_controls_run_after_power_on(void) {
    // EN: Set up GPIO0 backlight PWM. Keep the PWM timing below for this demo.
    // 中文：设置 GPIO0 背光 PWM。本 demo 保持下面的 PWM 时序参数。
    gpio_set_function(BACKLIGHT_PWM_PIN, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(BACKLIGHT_PWM_PIN);
    pwm_set_clkdiv(slice, 1.0f);
    pwm_set_wrap(slice, 255);

    // EN: Edit 180 to adjust PWM duty: use a value from 0 to 256.
    //     0 = always LOW, 256 = always HIGH; 180 is about 70% HIGH duty.
    // 中文：修改 180 可调整 PWM 占空比，请使用 0 到 256 之间的数值。
    //       0 为恒低，256 为恒高，180 约为 70% 高电平占空比。
    pwm_set_chan_level(slice, PWM_CHAN_A, 180);
    pwm_set_enabled(slice, true);

    bool fan_on = false;

    while (true) {
        // EN: Convert one ADC reading to volts. This is not Celsius.
        //     The calculation assumes a 3.3 V ADC reference.
        // 中文：将一次 ADC 读数换算为电压，不是摄氏温度。
        //       此公式使用 3.3 V 作为 ADC 参考电压。
        uint16_t raw = adc_read();
        float voltage = raw * 3.3f / 4095.0f;

        // EN: To view printf logs over USB, enable USB stdio in CMakeLists.txt.
        // 中文：如需通过 USB 查看 printf 日志，请在 CMakeLists.txt 开启 USB stdio。
        printf("Thermistor Voltage: %.2f V\n", voltage);

        // EN: Above ON, start the fan. Below OFF, stop it. Otherwise keep its state.
        //     This demonstrates fan ON/OFF control, not speed adjustment.
        // 中文：高于开启阈值时启动，低于关闭阈值时停止，其余情况保持原状态。
        //       这里演示风扇开关控制，不是转速调节。
        if (!fan_on && voltage > FAN_ON_THRESHOLD_V) {
            gpio_put(FAN_CTRL_HIGH_PIN, 1);
            gpio_put(FAN_CTRL_LOW_PIN, 0);
            fan_on = true;
            printf("FAN ON\n");
        } else if (fan_on && voltage < FAN_OFF_THRESHOLD_V) {
            gpio_put(FAN_CTRL_HIGH_PIN, 0);
            gpio_put(FAN_CTRL_LOW_PIN, 0);
            fan_on = false;
            printf("FAN OFF\n");
        }

        // EN: Check about once per second. Edit 1000 to change the interval in ms.
        // 中文：约每秒检查一次，修改 1000 可调整间隔，单位为毫秒。
        sleep_ms(1000);
    }
}
