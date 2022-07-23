#include <Math.h>
#include <Arduino.h>

class TempColor {
  public:
    int A;
    int R;
    int G;
    int B;
    // Class constructor
    TempColor(int alpha, int red, int green, int blue) {
        A = alpha;
        R = red;
        G = green;
        B = blue;
    }
    // default constructor
    TempColor() {}
};

class ColorConverter {

    public:
        static TempColor GradientPick(double percentage, TempColor Start, TempColor End);
        static String RGBtoHEX(int r, int g, int b);
};