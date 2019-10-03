bool useNeopixel = 0;


// **** Make neopixel object
// if rgbw use top line, if rgb use second.
//Adafruit_NeoPixel strip = Adafruit_NeoPixel(8, neoStripPin, NEO_GRBW + NEO_KHZ800);
//Adafruit_NeoPixel strip = Adafruit_NeoPixel(30, neoStripPin, NEO_GRB + NEO_KHZ800);
uint32_t maxBrightness = 255;


// make a loadcell object and set variables
#define calibration_factor 440000
#define zero_factor 8421804

//HX711 scale(scaleData, scaleClock);
uint32_t weightOffset = 0;
float scaleVal = 0;
