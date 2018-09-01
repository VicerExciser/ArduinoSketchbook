// Macro operations are much faster than functions & don't need to allocate stack space
//   #define ABS(x) ((x)>0?(x):-(x))
//   #define constrain(amt,low,high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))
//   #define round(x) ((x)>=0?(int)((x)+0.5):(int)((x)-0.5))

// If numSensors and/or numLights is power of 2, use right-shift (x >> 8 == x/256)
//  for any averaging division functions

// ONLY initialize variables if setting to a non-zero value, else wasting flash ROM space
// `byte` data type == uint8_t (or unsigned char)
// `word` data type == unsigned int (still 4 bytes; try using `uint16_t` instead)

// Note that I use #defines in this code (despite being depricated according to Arduino)
//  because macros are pre-processed and not compiled, thus they take up no Flash/program memory;
//  values are replaced inline w/ instances of use, so if a macro is not used then it will
//  consume no memory (as opposed to `const` variables).

#define NUMVPS    1     // # of monitored Velostat pressure sensors   (will be 4)
#define NUMLED    1     // # of monitored LED strips                  (will be 2)

#define VPSPIN1   A0    // Velostat pressure sensor analog pin #1
// ... more VPS pin defines to go here
#define RPIN1     5     // Red pin for LED strip #1
#define GPIN1     6     // Green pin for LED strip #1
#define BPIN1     3     // Blue pin for LED strip #1
// ... more RGB pin defines to go here

const byte led1[] = {RPIN1, GPIN1, BPIN1};

// Index macros for accessing specific color value array in 1st dimension of colorPresets:
#define WHITE   (int)0
#define RED     (int)1
#define LIME    (int)2
#define BLUE    (int)3
#define YELLOW  (int)4
#define CYAN    (int)5
#define MAGENTA (int)6
#define SILVER  (int)7
#define GRAY    (int)8
#define MAROON  (int)9
#define OLIVE   (int)10
#define GREEN   (int)11
#define PURPLE  (int)12
#define TEAL    (int)13
#define NAVY    (int)14

// Index macros for accessing the Red, Green, or Blue component value in 2nd dimension of 
// the colorPresets matrix (per color assigned to 1st index value):
#define R       (int)0
#define G       (int)1
#define B       (int)2

// Matrix of RGB-value color presets, stored in Flash ROM to save precious SRAM
const int numColors = 15;
const PROGMEM byte colorPresets[][3] = {
//    { R , G , B },    
      {255,255,255},    // white
      {255,0,0},        // red
      {0,255,0},        // lime
      {0,0,255},        // blue
      {255,255,0},      // yellow
      {0,255,255},      // cyan
      {255,0,255},      // magenta
      {192,192,192},    // silver
      {128,128,128},    // gray
      {128,0,0},        // maroon
      {128,128,0},      // olive
      {0,128,0},        // green
      {128,0,128},      // purple 
      {0,128,128},      // teal
      {0,0,128}         // navy
      };
const byte off[] = {0,0,0};

byte curRGB1[3];    // current color of LED strip #1
// ... more color state pointers to go here

// Number of sensor samples to maintain a record of for smoothing (a rolling average).
//  The greater this value is, the more the readings will be smoothed,
//  but the slower the output will respond to the input.
#define SAMPLES 32    // intentionally a power of 2 (for right-shift division)
byte readsVPS1[SAMPLES];
// ... more sensor read data collections to go here

const uint16_t uShortOverflow = ((uint16_t)(pow(2,16)-1));
const byte errThreshold = 10;
const byte fadeSpeed = 10;
const byte loopDelay = 50;      // ms
uint16_t loopCount;

void setup() {
  Serial.begin(115200);
  pinMode(VPSPIN1, INPUT_PULLUP);
  for (byte pin = 0; pin < 3; pin++) {
    pinMode(led1[pin], OUTPUT);
  }
}

void loop() {
//  static uint16_t sum1 = 0;    // running total for VPS #1's sensor readings
//  static byte readIndx1 = 0;
  int rawVPS1 = analogRead(VPSPIN1);

  byte lum1 = process(rawVPS1);   // processes pressure input returns luminance (brightness)
  // ^ this is the average collected input value at this point
  if(lum1 <= errThreshold) {
    lum1 = 0;
  }
  
  int hue = PURPLE;          // color purple arbitrarily chosen for demonstration
  setColor(hue, lum1, led1); // demo only for VPS #1 && LED #1

  loopCount = loopCount < uShortOverflow ? ++loopCount : 0;
  delay(loopDelay);
}

// Processes raw input data thru:
//  (1) compressing initial type granularity
//  (2) inversely mapping the [0:1023] raw input to its [0:255] 8-bit value
//  (3) heuristically smoothing data value via rolling average computation
byte process(int rawRead) {
  byte outVal = (255 - (rawRead >> 2));         // divide by 4 to convert 32-bit to 8-bit value
  return smooth(rawRead, ((outVal > 0) ? outVal : -(outVal)));   // return smoothed abs value of diff.
  // ^ avoiding use of built-in abs() function; does not handle arithmetic within arg
}
// ^^ Should probably combine process() + smooth() ...

// TODO: refactor to account for which VPS this data originated 
//  (currently only accounts for VPS #1)
byte smooth(int rawRead, byte newRead) {
  static uint16_t sum1 = 0;    // running total for VPS #1's sensor readings
  static byte readIndx1 = 0;
  // ^ TODO: possibly make structs w/ this sort of metadata for each VPS
  static byte entries1 = 0;
  byte avg1 = 0;

  if(newRead < errThreshold) {
    newRead = 0;
  }

  readIndx1 %= SAMPLES;
  sum1 -= readsVPS1[readIndx1];
  readsVPS1[readIndx1] = constrain(newRead, 0, 255);
  sum1 += readsVPS1[readIndx1];
  readIndx1++;
  entries1++;
  if(entries1 < SAMPLES) {
    avg1 = sum1 / entries1;
  }
  else {
    avg1 = sum1 >> 5;   // == (sum1 / SAMPLES)
  }

  if(!(loopCount % 25)) {
    printReadings(rawRead, newRead, avg1);
  }
  
  return avg1;
}


// TODO: Need to fix how color should ramp up/down with the intensity/brightness value
//   AS IS, BRIGHTNESS VALUE ISNT EVEN BEING USED WHAT THE FUCK AUSTIN


// Designed to handle multiple, independent LED strips (if necessary)
void setColor(int color, byte brightness, const byte pins[]) {
  byte r = pgm_read_byte(&colorPresets[color][R]);
  byte g = pgm_read_byte(&colorPresets[color][G]);
  byte b = pgm_read_byte(&colorPresets[color][B]);

  if(curRGB1[0] < r) {
    for(byte i = curRGB1[0]; i < r; ++i) {
      analogWrite(pins[0], i);
      delay(fadeSpeed);
    }
  }
  else if(curRGB1[0] > r) {
    for(byte i = curRGB1[0]; i > r; --i) {
      analogWrite(pins[0], i);
      delay(fadeSpeed);
    }
  }
//  analogWrite(pins[0], r);
  if(curRGB1[1] < g) {
    for(byte j = curRGB1[1]; j < g; ++j) {
      analogWrite(pins[1], j);
      delay(fadeSpeed);
    }
  }
  else if(curRGB1[1] > g) {
    for(byte j = curRGB1[1]; j > g; --j) {
      analogWrite(pins[1], j);
      delay(fadeSpeed);
    }
  }
//  analogWrite(pins[1], g);
  if(curRGB1[2] < b) {
    for(byte k = curRGB1[2]; k < b; ++k) {
      analogWrite(pins[2], k);
      delay(fadeSpeed);
    }
  }
  else if(curRGB1[2] > b) {
    for(byte k = curRGB1[2]; k > b; --k) {
      analogWrite(pins[2], k);
      delay(fadeSpeed);
    }
  }
//  analogWrite(pins[2], b);

  if((curRGB1[0]!=r || curRGB1[1]!=g || curRGB1[2]!=b) && (r || g || b)) {
//    printColor(r, g, b);
    printColor(color);
  }
  
  curRGB1[0] = r;
  curRGB1[1] = g;
  curRGB1[2] = b;
}


void printReadings(int raw, byte out, byte avg) {
  Serial.print(F("\n  Sensor: "));
  Serial.print(raw);
  Serial.print(F("\t|\tOutput: "));
  Serial.println(out);
  Serial.print(F("~> Average: "));
  Serial.println(avg);
}

//void printColor(byte red, byte green, byte blue) {
void printColor(int indx) {
  Serial.print(F(" [Color - "));
  switch(indx) {
    case 0: Serial.print("WHITE");
      break;
    case 1: Serial.print("RED");
      break;
    case 2: Serial.print("LIME");
      break;
    case 3: Serial.print("BLUE");
      break;
    case 4: Serial.print("YELLOW");
      break;
    case 5: Serial.print("CYAN");
      break;
    case 6: Serial.print("MAGENTA");
      break;
    case 7: Serial.print("SILVER");
      break;
    case 8: Serial.print("GRAY");
      break;
    case 9: Serial.print("MAROON");
      break;
    case 10: Serial.print("OLIVE");
      break;
    case 11: Serial.print("GREEN");
      break;
    case 12: Serial.print("PURPLE");
      break;
    case 13: Serial.print("TEAL");
      break;
    case 14: Serial.print("NAVY");
      break;
    default: Serial.print("FUCK_ALL");
  }
  Serial.println(F("]"));
}


//#############################################################
// COLOR EFFECTS
// for moonlight: consider sequencing shades of white, gray, silver, pale/dull yellow, etc
// for city tail-light streaking: pulse wavering intensities of red (e.g., 255 to 0 to 200 to 0 to 180 ...)

