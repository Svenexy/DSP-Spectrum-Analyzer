#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

#include <MatrixHardware_Teensy3_ShieldV1toV3.h>    
#include <SmartMatrix.h>
#include <FastLED.h>

#define COLOR_DEPTH 24                  // De kleuren diepte, 24 is goed voor algemeen gebruik
const uint16_t kMatrixWidth = 64;       // Paneel breedte
const uint16_t kMatrixHeight = 32;      // Paneel hoogte
const uint8_t kRefreshDepth = 36;       
const uint8_t kDmaBufferRows = 4;       // Is bedoeld om een rowbuffer te hebben. 4 zorgt ervoor dat er meer RAM wordt gebruikt maar er is minder frame drops
const uint8_t kPanelType = SM_PANELTYPE_HUB75_32ROW_MOD16SCAN;  // Het type display wat wij gebruiken
// Extra instellingen
const uint32_t kMatrixOptions = (SM_HUB75_OPTIONS_NONE);
const uint8_t kBackgroundLayerOptions = (SM_BACKGROUND_OPTIONS_NONE);

// Alle instellingen toewijzen aan het paneel en een tekenlaag toevoegen waar straks alles op wordt getekend
SMARTMATRIX_ALLOCATE_BUFFERS(matrix, kMatrixWidth, kMatrixHeight, kRefreshDepth, kDmaBufferRows, kPanelType, kMatrixOptions);
SMARTMATRIX_ALLOCATE_BACKGROUND_LAYER(backgroundLayer, kMatrixWidth, kMatrixHeight, COLOR_DEPTH, kBackgroundLayerOptions);

#define LEVEL_AMOUND 32
// Audio configureren
AudioInputAnalog         Audio_in(A2); // Audio input pin A2.
AudioAnalyzeFFT1024      FFT_1024; // FFT met 1024 samples initialiseren.
AudioConnection          audioConnection(Audio_in, FFT_1024); // Audio en FFT aan elkaar koppelen.

// Omdat de getallen die uit de FFT komen van 0.000 tot 1.000 kunnen is er een scale nodig om het hele scherm te vullen
float scale = 128.0;
// Een Aray om alle gelezen waardes op te slaan
float level[LEVEL_AMOUND];
// Arrays die bij houden wat er op het scherm nu zichtbaar is.
// Hier kunnen we dan ook de bars langzaam laten zakken als het geluid lager wordt.
int shownFallingBars[LEVEL_AMOUND];
int shownAudioBars[LEVEL_AMOUND];

// Kleuren instellen
const SM_RGB black = CRGB(0, 0, 0);
const SM_RGB white = CRGB(255, 255, 255);
const SM_RGB red = CRGB(255, 0, 0);

void setup() {
  Serial.begin(115200);
  // Paneel initialiseren
  matrix.addLayer(&backgroundLayer);
  matrix.begin();
  // Paneel helderheid
  matrix.setBrightness(255);
  // Audio geheugen instellen
  AudioMemory(12);
}

void loop() {
  if (FFT_1024.available()) {
    level[0] =  FFT_1024.read(0, 1);
    level[1] =  FFT_1024.read(1, 2);
    level[2] =  FFT_1024.read(2);
    level[3] =  FFT_1024.read(3);
    level[4] =  FFT_1024.read(4);
    level[5] =  FFT_1024.read(5);
    level[6] =  FFT_1024.read(6, 7);
    level[7] =  FFT_1024.read(7, 8);
    level[8] =  FFT_1024.read(8, 9);
    level[9] =  FFT_1024.read(9, 11);
    level[10] = FFT_1024.read(11, 13);
    level[11] = FFT_1024.read(13, 15);
    level[12] = FFT_1024.read(15, 18);
    level[13] = FFT_1024.read(18, 22);
    level[14] = FFT_1024.read(22, 26);
    level[15] = FFT_1024.read(26, 31);
    level[16] = FFT_1024.read(31, 37);
    level[17] = FFT_1024.read(37, 44);
    level[18] = FFT_1024.read(44, 52);
    level[19] = FFT_1024.read(52, 62);
    level[20] = FFT_1024.read(62, 74);
    level[21] = FFT_1024.read(74, 88);
    level[22] = FFT_1024.read(88, 105);
    level[23] = FFT_1024.read(105, 125);
    level[24] = FFT_1024.read(125, 149);
    level[25] = FFT_1024.read(149, 177);
    level[26] = FFT_1024.read(177, 211);
    level[27] = FFT_1024.read(211, 251);
    level[28] = FFT_1024.read(251, 299);
    level[29] = FFT_1024.read(299, 356);
    level[30] = FFT_1024.read(356, 424);
    level[31] = FFT_1024.read(424, 505);

    // Print alle level waardes
    for(int i = 0; i <= LEVEL_AMOUND; i++) {
      Serial.print(level[i]);
      Serial.print(" ");
    }
    Serial.println();


    // Led paneel zwart maken
    backgroundLayer.fillScreen(black);

    // Displayed alle hoogtes van de levels op het led paneel
    for (int i = 0; i < LEVEL_AMOUND; i++) {
      int falling_bars = level[i] * scale;
      int audio_bars = level[i] * scale;

      // Controle of de falling_bars niet hoger wordt dan de scherm hoogte
      if (falling_bars >= kMatrixHeight) falling_bars = kMatrixHeight - 1;

      // Wanneer de gelezen waarde hoger is dan weergegeven set gelezen waarde.
      // Anders doe weergegeven waarde -1
      if (falling_bars >= shownFallingBars[i]) {
        shownFallingBars[i] = falling_bars;
      } else {
        if (shownFallingBars[i] > 0) 
          shownFallingBars[i] = shownFallingBars[i] - 1;
        falling_bars = shownFallingBars[i];
      }

      //Controle of de audio_bars niet hoger wordt dan de scherm hoogte
      if (audio_bars >= kMatrixHeight) audio_bars = kMatrixHeight - 1;

      // Wanneer de gelezen waarde hoger is dan weergegeven set gelezen waarde.
      // Anders doe weergegeven waarde -3
      if (audio_bars >= shownAudioBars[i]) {
        shownAudioBars[i] = audio_bars;
      } else {
        if (shownAudioBars[i] > 0)  
          shownAudioBars[i] = shownAudioBars[i] - 3;
        audio_bars = shownAudioBars[i];
      }

      // Kleur instelling
      SM_RGB color = CRGB(CHSV(126 - (i * 7) + i, 255, 255));

      // 
      if (shownFallingBars[i] >= 0 && shownAudioBars[i] >= 0) {
        // Rechthoek tonen op led paneel
        backgroundLayer.drawRectangle(i * 2, (kMatrixHeight - 1) - audio_bars, i * 2 + 1, kMatrixHeight - 1, color);
        // 
        for (int j = 0; j < kMatrixWidth / LEVEL_AMOUND; j++) {
          backgroundLayer.drawPixel(i * 2 + j, (kMatrixHeight - 1) - falling_bars, white);
        }
      }
    }
    
    backgroundLayer.swapBuffers();
//    matrix.countFPS();
  }
}
