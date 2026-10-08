#include <Arduino.h>
#include "alarm_scheduler.h"
#include "curtain_controller.h"
#include "manual_input.h"
#include "network_clock.h"

CurtainController controller;
ManualInput manualInput;
NetworkClock networkClock;
AlarmScheduler alarmScheduler;

void setup() {
  Serial.begin(115200);
  controller.begin();
  manualInput.begin();
  alarmScheduler.begin();
  networkClock.begin();
  Serial.println("Ready: hold manual button to run; release to stop");
}

void loop() {
  manualInput.update(controller);
  controller.update();
  networkClock.update(controller.isMoving());
  alarmScheduler.update(controller, manualInput);
  // Software step timing still needs physical validation.
  if (!controller.isMoving()) {
    delay(1);
  }
}
