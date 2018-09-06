// #####  (VPS == Velostat Pressure Sensor)  #####

//   Vcc  ~>  Red/Blue wires  (tied together)
//   Gnd  ~>  Black/Yellow wires  (alternating down column as Y/B/Y/B)

#define R          0
#define G          1
#define B          2
#define NUM_VPS    4  //4   // # of monitored Velostat pressure sensors
#define NUM_LED    2  //2   // # of monitored LED strips
#define NUM_AVG    32   // # of processed averages to track for smoothing
//                  ^ a power of 2 for faster division (bit-shift)
#define AVG_SHIFT  5
#define VPS_PIN1   A2    // Velostat pressure sensor analog pin #1
#define VPS_PIN2   A3    // Velostat pressure sensor analog pin #2
#define VPS_PIN3   A1    // Velostat pressure sensor analog pin #3
#define VPS_PIN4   A0    // Velostat pressure sensor analog pin #4
#define R_PIN1     5     // Red pin for LED strip #1
#define G_PIN1     6     // Green pin for LED strip #1
#define B_PIN1     3     // Blue pin for LED strip #1
#define R_PIN2     9     // Red pin for LED strip #2
#define G_PIN2     10     // Green pin for LED strip #2
#define B_PIN2     11     // Blue pin for LED strip #2
#define RGB_MAX    255
#define LARGE_PAD  1 //0

const int largePadThreshMin = 460;    // raise thresh if flicker occurs
const int largePadThreshMax = 1023;
const int smallPadThreshMin = 30;     // raise thresh if flicker occurs
const int smallPadThreshMax = 850;
const int leds[][3] = {
    {R_PIN1, G_PIN1, B_PIN1}
     ,
     {R_PIN2, G_PIN2, B_PIN2}
};
const uint8_t sensors[] = {VPS_PIN1, VPS_PIN2, VPS_PIN3, VPS_PIN4};
/*
const uint8_t gamma[] PROGMEM = { // Gamma correction table for LED brightness
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,
    1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,
    2,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  5,  5,  5,
    5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  9,  9,  9, 10,
   10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
   17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
   25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
   37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
   51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
   69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
   90, 92, 93, 95, 96, 98, 99,101,102,104,105,107,109,110,112,114,
  115,117,119,120,122,124,126,127,129,131,133,135,137,138,140,142,
  144,146,148,150,152,154,156,158,160,162,164,167,169,171,173,175,
  177,180,182,184,186,189,191,193,196,198,200,203,205,208,210,213,
  215,218,220,223,225,228,231,233,236,239,241,244,247,249,252,255 };
const uint8_t gammaLen = 256;
*/
uint8_t brightVals[NUM_VPS];
int averages[NUM_VPS][NUM_AVG], /*sums[NUM_VPS],*/ oldestAvg;
bool avgFull = false;   // True when len(averages[i]) == NUM_AVG
int vpsLastMax, vpsLastMin;

void setup() {
    Serial.begin(115200);
    for (int i = 0; i < NUM_VPS; i++) {
        pinMode(sensors[i], INPUT_PULLUP);
    }
    for (int j = 0; j < NUM_LED; j++) {
        for (int k = 0; k < 3; k++) {
            pinMode(leds[j][k], OUTPUT);
        }
    }
    memset(brightVals, 0, sizeof(brightVals));
    memset(averages, 0, sizeof(averages[0][0]) * NUM_VPS * NUM_AVG);
//    memset(sums, 0, sizeof(sums));
    vpsLastMin = 900;
}


void loop() {
  int sensVal, vpsUpperBound, vpsLowerBound, mag;
    for (int vps = 0; vps < NUM_VPS; vps++) {
//      int sensVal, vpsUpperBound, vpsLowerBound, mag;
      sensVal = analogRead(sensors[vps]);

      // For testing analog input value ranges:
      vpsUpperBound = max(sensVal, vpsLastMax);
      if (vpsUpperBound != vpsLastMax) {
          vpsLastMax = vpsUpperBound;
          Serial.print(F("++ VPS UPPER = "));
          Serial.println(vpsUpperBound);
      }
      vpsLowerBound = min(sensVal, vpsLastMin);
      if (vpsLowerBound != vpsLastMin) {
          vpsLastMin = vpsLowerBound;
          Serial.print(F("\t\t-- VPS LOWER = "));
          Serial.println(vpsLowerBound);
      }

      // A dynamic mapping mechanism:
      // mag = map(sensVal, vpsLastMin, vpsLastMax, 0, RGB_MAX);

      if (!LARGE_PAD) {
        mag = map(constrain(sensVal,smallPadThreshMin,smallPadThreshMax), 
            smallPadThreshMin, smallPadThreshMax, 0, RGB_MAX);
      } else {
        mag = map(constrain(sensVal,largePadThreshMin,largePadThreshMax), 
            largePadThreshMin, largePadThreshMax, 0, RGB_MAX);
      }
      constrain(mag, 0, RGB_MAX);
//        Serial.println(mag);

/*
        // Be sure to clamp mag to [0:255]
      sums[vps] -= averages[vps][oldestAvg];
       // sums[vps] += pgm_read_byte(&gamma[mag]);
      sums[vps] += mag;

      averages[vps][oldestAvg] = avgFull ?
              (sums[vps] >> AVG_SHIFT) : (sums[vps] / oldestAvg);
*/
    int sum = 0;
    averages[vps][oldestAvg] = mag;
    for (int i = 0; i < NUM_AVG; i++) {
         sum += averages[vps][i];
    }
    averages[vps][oldestAvg] = avgFull ?
              (sum >> AVG_SHIFT) : (sum / oldestAvg);

      brightVals[vps] = byte(constrain(averages[vps][oldestAvg], 0, RGB_MAX));
//        Serial.println(brightVals[vps]);
    }
    
    oldestAvg++;
    if (!avgFull) {
        avgFull = oldestAvg == NUM_AVG;
    }

    if (oldestAvg == NUM_AVG) oldestAvg = 0;  // Faster than modulo

//    analogWrite(R_PIN2, 0);
//    analogWrite(R_PIN1, 255 - max(brightVals[0], brightVals[1]));

      analogWrite(R_PIN1, brightVals[0]);
//    analogWrite(G_PIN1, max(brightVals[1], brightVals[2]));
      
      analogWrite(B_PIN1, brightVals[1]);
//    analogWrite(G_PIN2, max(brightVals[0], brightVals[1]));
      
      analogWrite(R_PIN2, brightVals[2]);

      analogWrite(B_PIN2, brightVals[3]);

//    printReadings();
   
//    delay(loopDelay);
}

/*
void printReadings() {
    for (int i = 0; i < NUM_VPS; i++) {
//        Serial.print(i);
        // Serial.print(F("#\n\tValue: "));
        // Serial.print(sensVals[i]);
  // Serial.print(F("\t|\tOutput: "));
  // Serial.print(out);
        Serial.print(F("\n\t\t~> Brightness: "));
        Serial.println(brightVals[i]);
    }
    Serial.println(F("======================"));
}
*/

