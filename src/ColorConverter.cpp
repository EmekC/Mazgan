#include "ColorConverter.h"

TempColor FromArgb(int a, int r, int g, int b) {
  TempColor color(a, r, g, b);

  return color;
}  

int LinearInterp(int start, int end, double percentage) {
  return start + (int)round(percentage * (end - start));
} 

TempColor ColorInterp(TempColor start, TempColor end, double percentage) {
    return FromArgb(LinearInterp(start.A, end.A, percentage),
                   LinearInterp(start.R, end.R, percentage),
                   LinearInterp(start.G, end.G, percentage),
                   LinearInterp(start.B, end.B, percentage));
}

TempColor ColorConverter::GradientPick(double percentage, TempColor Start, TempColor End) {
    if (percentage <= 0.0) {
      return ColorInterp(Start, End, 0.1);
    } else {
        return ColorInterp(Start, End, percentage);
    } 
}

String ColorConverter::RGBtoHEX(int r, int g, int b) {
  char hexArray[6] = { 0 };
	sprintf(hexArray, "%02X%02X%02X", r, g, b);
	String hexColor = hexArray;
  return hexColor;
}
