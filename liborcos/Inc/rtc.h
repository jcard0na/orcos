#pragma once

extern RTC_HandleTypeDef hrtc;
void WakeUpTimerEventCallback(RTC_HandleTypeDef *hrtc) __attribute__((used, noinline));