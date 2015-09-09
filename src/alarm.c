#include "alarm.h"

void convert_24_to_12(int hour_in, int* hour_out, bool* am)
{
  *hour_out=hour_in%12;
  if (*hour_out==0) {
    *hour_out=12;
  }
  *am=hour_in<12;
}