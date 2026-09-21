#include "ui_queue.hpp"

QueueHandle_t UiQueue::queue = nullptr;

void UiQueue::init() 
{
  if (queue == nullptr) 
  {
    queue = xQueueCreate(QUEUE_LENGTH, ITEM_SIZE);
    // Do we know if it fails? Can it fail?
  }
}

int UiQueue::send(Ui_Data const& data) 
{
  if (queue == nullptr) 
    return 1;

  if (xQueueSend(queue, &data, portMAX_DELAY) != pdPASS)
    return 2;

  return 0;
}

int UiQueue::receive(Ui_Data& out_data, TickType_t timeout_ticks) 
{
  if (queue == nullptr) 
    return 1;

  if (xQueueReceive(queue, &out_data, timeout_ticks) != pdPASS)
    return 2;

  return 0;
}
