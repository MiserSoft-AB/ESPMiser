#ifndef __LEOP_DATA_HPP__
#define __LEOP_DATA_HPP__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdlib.h>

typedef struct 
{
  float    price;

} Leop_Data; // Replace with actual struct from shared lib

typedef struct 
{
  Leop_Data   leop_data;
  // uint32_t    timestamp_ms;
  // bool        has_leop;
} Ui_Data;

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __LEOP_DATA_HPP__
