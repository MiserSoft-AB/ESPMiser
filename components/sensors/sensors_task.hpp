#ifndef __SENSORS_TASK_HPP__
#define __SENSORS_TASK_HPP__

class Sensors {
public:
  // You better pass a QueueHandle_t param or else! 
  static void sensors_task_test(void* param);
};

#endif // __SENSORS_TASK_HPP__
