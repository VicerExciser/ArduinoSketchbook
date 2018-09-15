// #######  (VPS == Velostat Pressure Sensor)  #######
// Author:  Austin Condict

#define VPS_PIN1   A0    // Velostat pressure sensor analog pin #00
#define VPS_PIN2   A1    // Velostat pressure sensor analog pin #01
#define VPS_PIN3   A2    // Velostat pressure sensor analog pin #02
#define VPS_PIN4   A3    // Velostat pressure sensor analog pin #03
#define R_PIN1     3     // Red pin for LED strip #1
#define G_PIN1     5     // Green pin for LED strip #1
#define B_PIN1     6     // Blue pin for LED strip #1
#define R_PIN2     9     // Red pin for LED strip #2
#define G_PIN2     10    // Green pin for LED strip #2
#define B_PIN2     11    // Blue pin for LED strip #2
#define NUM_VPS    4     // # of monitored Velostat pressure sensors
#define NUM_LED    2     // # of monitored LED strips
#define NUM_AVG    32    // # of processed averages to track for smoothing
#define AVG_SHIFT  5
#define RGB_MAX    255
#define R          0
#define G          1
#define B          2
#define FINISH     26
#define POST       27
 const unsigned long tIntermission1 = 30000;      // 8 of these  [30 sec]
 const unsigned long tIntermission2 = 23000;      // 5 of these [23 sec] (including finale)
 const unsigned long tSolid1 = 285000;  // 9 of these [4 min 45 sec]
 const unsigned long tSolid2 = 170000;  // 4 of these [2 min 50 sec]
//const unsigned long tIntermission1 = 10000;    //  10 sec - test
//const unsigned long tIntermission2 = 5000;    //  5 sec - test
//const unsigned long tSolid1 = 20000;  //    20 sec - test
//const unsigned long tSolid2 = 15000;  //    15 sec - test
const unsigned long longInterDelayT = 60; //61;
const unsigned long shortInterDelayT = 49; //50;
const int largePadThreshMin = 465;    // raise thresh if flicker occurs
const int largePadThreshMax = 975;
const int leds[][3] = {{R_PIN1, G_PIN1, B_PIN1},{R_PIN2, G_PIN2, B_PIN2}};
const uint8_t sensors[] = {VPS_PIN1, VPS_PIN2, VPS_PIN3, VPS_PIN4};
unsigned long lastTimestamp, changeTime;
int averages[NUM_VPS][NUM_AVG], oldestAvg, state, h1i, h2i;
uint8_t brightVals[NUM_VPS];
byte r1, r2, g1, g2, b1, b2;
bool avgFull = false;
bool w2a = true;

void setup() {
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
    state = 0;
    changeTime = tSolid1;
    h2i = 1;
    lastTimestamp = millis();
}

void loop() {
    int sensVal, mag, sum;
    unsigned long now = millis();
    
    if ((now - lastTimestamp) >= changeTime) {
        lastTimestamp = millis();
        if (state <= FINISH) {
            state++;
            if (state < 16) {
                if (state % 2 == 0) {
                    clearColors();
                    changeTime = tSolid1;
                } else {
                    changeTime = tIntermission1;
                    w2a = !w2a;
                }
            } else if (state == FINISH) {
                flashWhite(500);
            } else {
                if (state % 2 == 0) {
                    clearColors();
                  changeTime = tSolid2;
                } else {
                  changeTime = tIntermission2;
                  w2a = !w2a;
                }
            } 
        }
        swapHueIndx();
    }

  for (int vps = 0; vps < NUM_VPS; vps++) {
    sensVal = analogRead(sensors[vps]);
    mag = map(constrain(sensVal,largePadThreshMin,largePadThreshMax),
          largePadThreshMin, largePadThreshMax, 0, RGB_MAX);
    mag = constrain(mag, 0, RGB_MAX);
    if (mag < 10) mag = 0;
    sum = 0;
    averages[vps][oldestAvg] = mag;
    for (int i = 0; i < NUM_AVG; i++) {
         sum += averages[vps][i];
    }
    averages[vps][oldestAvg] = avgFull ?
            (sum >> AVG_SHIFT) : (sum / oldestAvg);

    brightVals[vps] = byte(constrain(averages[vps][oldestAvg], 0, RGB_MAX));
  }
  oldestAvg++;
  if (!avgFull) avgFull = oldestAvg == NUM_AVG;
  if (oldestAvg == NUM_AVG) oldestAvg = 0;  

byte
    first = brightVals[0],
    second = brightVals[1],
    third = brightVals[2],
    fourth = brightVals[3],
    max12 = max(first,second),
    max13 = max(first,third),
    max14 = max(first,fourth),
    max23 = max(second,third),
    max24 = max(second,fourth),
    max34 = max(third,fourth);

  if (state > FINISH) {
    analogWrite(leds[h1i][R], first);
    analogWrite(leds[h2i][B], second);
    analogWrite(leds[h1i][G], third);
    analogWrite(leds[h2i][R], third);
    analogWrite(leds[h1i][B], fourth);
    analogWrite(leds[h2i][G], fourth);
  } else if (state % 2 == 0) {
      if (state == 0 || state == 12) { 
        // Blue & Wine
        analogWrite(leds[h1i][B], max(max14,max23)>>1);
        analogWrite(leds[h1i][R], max14);
        analogWrite(leds[h2i][B], max(max13, max24)>>1);
        analogWrite(leds[h2i][R], max13);
      } else if (state == 2 || state == 14) {
        // Green & Amber
        analogWrite(leds[h1i][G], max(max14,max23)>>1);
        analogWrite(leds[h1i][R], max14);
        analogWrite(leds[h2i][G], max(max13,max24)>>1);
        analogWrite(leds[h2i][R], max13);
      } else if (state == 4 || state == 16) {
          // Red & Wine
        analogWrite(leds[h1i][R], max(max13,max24));
        analogWrite(leds[h1i][B], second>>1);
        analogWrite(leds[h2i][R], max(max13, max24));
        analogWrite(leds[h2i][B], fourth>>1);
      } else if (state == 6) {
        // Blue & Amber
        analogWrite(leds[h1i][B], max13);
        analogWrite(leds[h1i][G], max24>>1);
        analogWrite(leds[h1i][R], max24);
        analogWrite(leds[h2i][B], max13);
        analogWrite(leds[h2i][G], max24>>1);
        analogWrite(leds[h2i][R], max24);
      } else if (state == 8) {
        // Green & Wine 
        analogWrite(leds[h1i][G], max24);
        analogWrite(leds[h1i][R], max13);
        analogWrite(leds[h1i][B], max13>>1);
        analogWrite(leds[h2i][G], max13);
        analogWrite(leds[h2i][B], max24>>1);
        analogWrite(leds[h2i][R], max24);
      } else if (state == 10) {
        // Red & Amber
        analogWrite(leds[h1i][R], max(max14,max23));
        analogWrite(leds[h1i][G], max14>>1);
        analogWrite(leds[h2i][R], max(max13,max24));
        analogWrite(leds[h2i][G], max13>>1);
      } else if (state == 18) {
        // Red & Blue & Amber
        analogWrite(leds[h1i][R], max13);
        analogWrite(leds[h1i][B], max24);
        analogWrite(leds[h1i][G], max23>>1);
        analogWrite(leds[h2i][R], max13);
        analogWrite(leds[h2i][B], max24);
        analogWrite(leds[h2i][G], max23>>1);
      } else if (state == 20) {
         // Green & Amber + Blue & Wine
        analogWrite(leds[h1i][G], max34);
        analogWrite(leds[h1i][R], fourth);
        analogWrite(leds[h2i][G], third);
        analogWrite(leds[h1i][B], first);
        analogWrite(leds[h2i][B], max12);
        analogWrite(leds[h2i][R], second);
      } else if (state == 22) {
        analogWrite(leds[h1i][G], max34>>1);
          analogWrite(leds[h1i][R], max23/*24*/);
          analogWrite(leds[h2i][G], max13);
          analogWrite(leds[h1i][B], max14);
          analogWrite(leds[h2i][B], max12>>1);
          analogWrite(leds[h2i][R], max23);
      } else if (state == 24) {
        analogWrite(leds[h1i][R], max12);
        analogWrite(leds[h2i][R], max13);
        analogWrite(leds[h1i][B], max14);
        analogWrite(leds[h2i][B], max23);
        analogWrite(leds[h1i][G], max24);
        analogWrite(leds[h2i][G], max34);
      }
  } else {
    int fade = changeTime == tIntermission1 
            ? longInterDelayT : shortInterDelayT;
    if (state == 1 || state == 7 || state == 13) {
        // Blue -> Green
        if (w2a) {
            r1 = 0;
            g1 = 0;
            b1 = RGB_MAX;
            r2 = RGB_MAX;
            g2 = 0;
            b2 = RGB_MAX-200;
            while (g1 < RGB_MAX || b1 > 0
                || r2 < RGB_MAX || g2 < 128 || b2 > 0) {
              analogWrite(leds[h1i][R], r1);
              analogWrite(leds[h1i][G], g1);
              analogWrite(leds[h1i][B], b1);
              if (g1 < RGB_MAX) g1++;
              if (b1 > 0) b1--;
              analogWrite(leds[h2i][R], r2);
              analogWrite(leds[h2i][G], g2);
              analogWrite(leds[h2i][B], b2);
              if (r2 < RGB_MAX) r2++;
              if (b2 > 0) b2--;
              if (g2 < 128) g2++;
              delay(fade);
            }
            delay(fade*2);
        } else {
            r1 = 0;
            g1 = 0;
            b1 = RGB_MAX;
            r2 = RGB_MAX;
            g2 = 128;
            b2 = 0;
            while (g1 < RGB_MAX || b1 > 0
                || r2 > RGB_MAX || b2 < RGB_MAX || g2 > 0) {
              analogWrite(leds[h1i][R], r1);
              analogWrite(leds[h1i][G], g1);
              analogWrite(leds[h1i][B], b1);
              if (g1 < RGB_MAX) g1++;
              if (b1 > 0) b1--;
              analogWrite(leds[h2i][R], r2);
              analogWrite(leds[h2i][G], g2);
              analogWrite(leds[h2i][B], b2);
              if (r2 > RGB_MAX) r2--;
              if (b2 < RGB_MAX) b2++;
              if (g2 > 0) g2--;
              delay(fade);
            }
            delay(fade*2);
        }
    } else if (state == 3 || state == 9 || state == 15) {
        // Green -> Red
        if (w2a) {
            r1 = 0;
            g1 = RGB_MAX;
            b1 = 0;
            r2 = RGB_MAX;
            g2 = 0;
            b2 = RGB_MAX-200;
            while (r1 < RGB_MAX || g1 > 0
                || r2 < RGB_MAX || g2 < 128 || b2 > 0) {
              analogWrite(leds[h1i][R], r1);
              analogWrite(leds[h1i][G], g1);
              analogWrite(leds[h1i][B], b1);
              if (r1 < RGB_MAX) r1++;
                if (g1 > 0) g1--;
              analogWrite(leds[h2i][R], r2);
              analogWrite(leds[h2i][G], g2);
              analogWrite(leds[h2i][B], b2);
              if (r2 < RGB_MAX) r2++;
              if (b2 > 0) b2--;
              if (g2 < 128) g2++;
              delay(fade);
            }
            delay(fade*2);
        } else {
            r1 = 0;
            g1 = RGB_MAX;
            b1 = 0;
            r2 = RGB_MAX;
            g2 = 128;
            b2 = 0;
            while (r1 < RGB_MAX || g1 > 0
                || r2 > RGB_MAX || b2 < RGB_MAX || g2 > 0) {
              analogWrite(leds[h1i][R], r1);
              analogWrite(leds[h1i][G], g1);
              analogWrite(leds[h1i][B], b1);
              if (r1 < RGB_MAX) r1++;
                if (g1 > 0) g1--;
              analogWrite(leds[h2i][R], r2);
              analogWrite(leds[h2i][G], g2);
              analogWrite(leds[h2i][B], b2);
              if (r2 > RGB_MAX) r2--;
              if (b2 < RGB_MAX) b2++;
              if (g2 > 0) g2--;
              delay(fade);
            }
            delay(fade*2);
        }
    } else if (state == 5 || state == 11) {
        // Red -> Blue
        if (w2a) {
            r1 = RGB_MAX;
            g1 = 0;
            b1 = 0;
            r2 = RGB_MAX;
            g2 = 0;
            b2 = RGB_MAX-200;
            while (r1 > 0 || b1 < RGB_MAX
                || r2 < RGB_MAX || g2 < 128 || b2 > 0) {
              analogWrite(leds[h1i][R], r1);
              analogWrite(leds[h1i][G], g1);
              analogWrite(leds[h1i][B], b1);
              if (b1 < RGB_MAX) b1++;
                if (r1 > 0) r1--;
              analogWrite(leds[h2i][R], r2);
              analogWrite(leds[h2i][G], g2);
              analogWrite(leds[h2i][B], b2);
              if (r2 < RGB_MAX) r2++;
              if (b2 > 0) b2--;
              if (g2 < 128) g2++;
              delay(fade);
            }
            delay(fade*2);
        } else {
            r1 = RGB_MAX;
            g1 = 0;
            b1 = 0;
            r2 = RGB_MAX;
            g2 = 128;
            b2 = 0;
            while (r1 > 0 || b1 < RGB_MAX
                || r2 > RGB_MAX || b2 < RGB_MAX || g2 > 0) {
              analogWrite(leds[h1i][R], r1);
              analogWrite(leds[h1i][G], g1);
              analogWrite(leds[h1i][B], b1);
              if (b1 < RGB_MAX) b1++;
                 if (r1 > 0) r1--;
              analogWrite(leds[h2i][R], r2);
              analogWrite(leds[h2i][G], g2);
              analogWrite(leds[h2i][B], b2);
              if (r2 > RGB_MAX) r2--;
              if (b2 < RGB_MAX) b2++;
              if (g2 > 0) g2--;
              delay(fade);
            }
            delay(fade*2);
        }
    } else if (state == 17) {
        // Wine -> Amber  + Blue
        fade = 47;
        r1 = 0;
        g1 = 0;
        b1 = RGB_MAX;
        r2 = RGB_MAX;
        g2 = 0;
        b2 = RGB_MAX-200;
        while (r2 < RGB_MAX || g2 < 128 || b2 > 0) {
          analogWrite(leds[h1i][R], r1);
          analogWrite(leds[h1i][G], g1);
          analogWrite(leds[h1i][B], b1);
          analogWrite(leds[h2i][R], r2);
          analogWrite(leds[h2i][G], g2);
          analogWrite(leds[h2i][B], b2);
          if (r2 < RGB_MAX) r2++;
          if (b2 > 0) b2--;
          if (g2 < 128) g2++;
          delay(fade);
        }
        delay(fade*2);
    } else if (state == 19) {
        // Red -> Green  (a2w)
        r1 = RGB_MAX;
        g1 = 0;
        b1 = 0;
        r2 = RGB_MAX;
        g2 = 128;
        b2 = 0;
        while (r1 > 0 || g1 < RGB_MAX
            || r2 > RGB_MAX || b2 < RGB_MAX || g2 > 0) {
              analogWrite(leds[h1i][R], r1);
              analogWrite(leds[h1i][G], g1);
              analogWrite(leds[h1i][B], b1);
              if (g1 < RGB_MAX) g1++;
                if (r1 > 0) r1--;
                analogWrite(leds[h2i][R], r2);
              analogWrite(leds[h2i][G], g2);
              analogWrite(leds[h2i][B], b2);
              if (r2 > RGB_MAX) r2--;
              if (b2 < RGB_MAX) b2++;
              if (g2 > 0) g2--;
              delay(fade);
            }
            delay(fade*2);
    } else if (state == 21) {
        // Blue -> Red  (w2a)
        r1 = 0;
        g1 = 0;
        b1 = RGB_MAX;
        r2 = RGB_MAX;
        g2 = 0;
        b2 = RGB_MAX-200;
        while (r1 < RGB_MAX || b1 > 0
            || r2 < RGB_MAX || g2 < 128 || b2 > 0) {
          analogWrite(leds[h1i][R], r1);
          analogWrite(leds[h1i][G], g1);
          analogWrite(leds[h1i][B], b1);
          if (r1 < RGB_MAX) r1++;
          if (b1 > 0) b1--;
            analogWrite(leds[h2i][R], r2);
            analogWrite(leds[h2i][G], g2);
            analogWrite(leds[h2i][B], b2);
              if (r2 < RGB_MAX) r2++;
              if (b2 > 0) b2--;
              if (g2 < 128) g2++;
          delay(fade);
        }
        delay(fade*2);
    } else if (state == 23) {
        fade = 445;
        analogWrite(leds[h1i][B], RGB_MAX);
        delay(fade);
        clearColors();
        analogWrite(leds[h2i][B], RGB_MAX);
        delay(fade);
        clearColors();
        analogWrite(leds[h1i][R], RGB_MAX);
        analogWrite(leds[h1i][B], RGB_MAX>>1);
        delay(fade);
        clearColors();
        analogWrite(leds[h2i][R], RGB_MAX);
        analogWrite(leds[h2i][B], RGB_MAX>>1);
        delay(fade);
        clearColors();
        analogWrite(leds[h1i][R], RGB_MAX);
        delay(fade*2);
        clearColors();
        analogWrite(leds[h2i][G], RGB_MAX);
        delay(fade*2);
        clearColors();
        analogWrite(leds[h1i][R], RGB_MAX);
        analogWrite(leds[h2i][R], RGB_MAX);
        analogWrite(leds[h1i][G], RGB_MAX-200);
        analogWrite(leds[h2i][G], RGB_MAX-200);
        delay(fade*3);
        clearColors();
    } else if (state == 25) {
        fade = 500;
        // Red
        analogWrite(leds[h1i][R], RGB_MAX);
        analogWrite(leds[h2i][R], RGB_MAX);
        delay(fade);
        clearColors();
        // Green
        analogWrite(leds[h1i][G], RGB_MAX);
        analogWrite(leds[h2i][G], RGB_MAX);
        delay(fade);
        clearColors();
        // Blue
        analogWrite(leds[h1i][B], RGB_MAX);
        analogWrite(leds[h2i][B], RGB_MAX);
        delay(fade);
        clearColors();
        // Wine
        analogWrite(leds[h1i][R], RGB_MAX);
        analogWrite(leds[h1i][R], RGB_MAX);
        analogWrite(leds[h2i][B], RGB_MAX>>1);
        analogWrite(leds[h2i][B], RGB_MAX>>1);
        delay(fade);
        clearColors();
        // Amber
        analogWrite(leds[h1i][R], RGB_MAX);
        analogWrite(leds[h1i][R], RGB_MAX);
        analogWrite(leds[h2i][G], RGB_MAX-200);
        analogWrite(leds[h2i][G], RGB_MAX-200);
        delay(fade);
        clearColors();
        // delay(fade*2);
    }
    swapHueIndx();
  }
}

void swapHueIndx() {
  if (h1i == 0) {
    h1i = 1;
    h2i = 0;
  } else {
    h1i = 0;
    h2i = 1;
  }
}

void flashWhite(unsigned long d) {
  clearColors();
  for (int i = 0; i < NUM_VPS; i++) {
     delay(d);
    analogWrite(leds[h1i][R],RGB_MAX);
    analogWrite(leds[h1i][G],RGB_MAX);
    analogWrite(leds[h1i][B],RGB_MAX);
    analogWrite(leds[h2i][R],RGB_MAX);
    analogWrite(leds[h2i][G],RGB_MAX);
    analogWrite(leds[h2i][B],RGB_MAX);
    delay(d);
    clearColors();
  }
  delay(d*10);
}

void clearColors() {
  for (int i = 0; i < NUM_LED; i++) {
    analogWrite(leds[i][R],0);
    analogWrite(leds[i][G],0);
    analogWrite(leds[i][B],0);
  }
}
