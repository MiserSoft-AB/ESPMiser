#ifndef __UI_TASK_HPP__
#define __UI_TASK_HPP__

class Ui 
{
public:
  // static int ui_init();
  static void ui_task(void* param);

private:
  Ui() {} // no instantiation, might reconsider
          // we should learn to use templates though
};

#endif // __UI_TASK_HPP__
