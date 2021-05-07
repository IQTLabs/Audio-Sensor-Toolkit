/* Edge Impulse Arduino examples
 * Copyright (c) 2020 EdgeImpulse Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all co
pies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

// If your target is limited in memory remove this macro to save 10K RAM
//#define EIDSP_QUANTIZE_FILTERBANK   0

/**
 * Define the number of slices per model window. E.g. a model window of 1000 ms
 * with slices per model window set to 4. Results in a slice size of 250 ms.
 * For more info: https://docs.edgeimpulse.com/docs/continuous-audio-sampling
 */
#define EI_CLASSIFIER_SLICES_PER_MODEL_WINDOW 3

/* Includes ---------------------------------------------------------------- */
#include <PDM.h>


//#include <edge-impulse-sdk/dsp/numpy.hpp>
#include <SPI.h>

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "RTClib.h"

RTC_PCF8523 rtc;


// Set the pins used
//#include <SD.h>
#include "SdFat.h"
SdFat SD;
// Date and time functions using a PCF8523 RTC connected via I2C and Wire lib

#define cardSelect 10
#define EI_CLASSIFIER_FREQUENCY 16000
#define EI_CLASSIFIER_SLICE_SIZE 16000

Adafruit_SSD1306 display = Adafruit_SSD1306(128, 32, &Wire);

// OLED FeatherWing buttons map to different pins depending on board:
// 32u4, M0, M4, nrf52840 and 328p
  #define BUTTON_A  9
  #define BUTTON_B  6
  #define BUTTON_C  5


uint32_t counter = 0;


void display_sending(const char *text)
{
  char line[100];
  display.clearDisplay();
  display.setCursor(0, 0);
  sprintf(line, "Sent: %s\n", text);
  display.print(line);
  sprintf(line, "counter: %d\n", counter);
  display.print(counter);
  display.display();
}

/** Audio buffers, pointers and selectors */
/** Audio buffers, pointers and selectors */
typedef struct
{
  signed short *buffers[2];
  unsigned char buf_select;
  unsigned char buf_ready;
  unsigned int buf_count;
  unsigned int n_samples;
} inference_t;

static inference_t inference;
static int samples_recorded = 0;

static signed short *sampleBuffer;
static int16_t sample_max = 0;
/**
 * @brief      Arduino setup function
 */
void setup()
{

  Serial.begin(115200);
  //while( !Serial) delay(10);


  Serial.println("Starting");

  delay(100);

  if (!rtc.begin())
  {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    abort();
  }
  if (!rtc.initialized() || rtc.lostPower())
  {
    Serial.println("RTC is NOT initialized, let's set the time!");
    // When time needs to be set on a new device, or after a power loss, the
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
    //
    // Note: allow 2 seconds after inserting battery or applying external power
    // without battery before calling adjust(). This gives the PCF8523's
    // crystal oscillator time to stabilize. If you call adjust() very quickly
    // after the RTC is powered, lostPower() may still return true.
  }
  rtc.start();

  // see if the card is present and can be initialized:
  if (!SD.begin(cardSelect))
  {
    Serial.println("Card init. failed!");
    while (1)
      ;
  }

  Serial.println("Finished SD Stuff");

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); // Address 0x3C for 128x32
  // Show image buffer on the display hardware.
  // Since the buffer is intialized with an Adafruit splashscreen
  // internally, this will display the splashscreen.
  display.display();
  pinMode(BUTTON_A, INPUT_PULLUP);
  pinMode(BUTTON_B, INPUT_PULLUP);
  pinMode(BUTTON_C, INPUT_PULLUP);

  display.clearDisplay();
  display.display();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("Welcome");
  display.display();
  if (microphone_inference_start(EI_CLASSIFIER_SLICE_SIZE) == false)
  {
    ei_printf("ERR: Failed to setup audio sampling\r\n");
    return;
  }
  Serial.println("Finished setup");
}

int mode = 0;

/**
 * @brief      Arduino main function. Runs the inferencing loop.
 */
void loop()
{
  char line[100];
    DateTime now = rtc.now();

  bool m = microphone_inference_record();
  if (!m)
  {
    ei_printf("ERR: Failed to record audio...\n");
    return;
  }
String label = "";
  if (mode ==0) 
  { 
    if (!digitalRead(BUTTON_A)) {
      mode=1;
      samples_recorded=0;
      counter++;
    }
    if (!digitalRead(BUTTON_B)) {
      mode=2;
      samples_recorded=0;
      counter++;
    }
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("a) Sound\n");
    display.print("b) Noise\n");
    display.display();
    
  }
  if (mode==1) 
  { 
    label = "sound";
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("Sound\n");
    sprintf(line, "%d\n", samples_recorded);
    display.print(line);
    display.print("c) Stop\n");
    sprintf(line, "%d\n", sample_max);
    display.print(line);
    display.display();
    if (!digitalRead(BUTTON_C)) {
      mode=0;
    }
  }

  if (mode==2) 
  { 
    label = "noise";
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("Noise\n");
    sprintf(line, "%d\n", samples_recorded);
    display.print(line);
    display.print("c) Stop\n");
    sprintf(line, "%d\n", sample_max);
    display.print(line);
    display.display();
    if (!digitalRead(BUTTON_C)) {
      mode=0;
    }
  }


    


    if (mode!=0) {
    
      File sample_file;
  
      String day_path = String("/recording/") + now.day();
      if (!SD.exists(day_path.c_str()))
      {
        SD.mkdir(day_path.c_str());
      }
      String hour_path = String("/recording/") + now.day() + "/" + now.hour();
      if (!SD.exists(hour_path.c_str()))
      {
        SD.mkdir(hour_path.c_str());
      }
      String filename = String(label +"_" + counter + ".raw");
  
      String full_path = hour_path + "/" + filename;
      Serial.print("Saving sample to file: ");
      Serial.println(full_path);
      sample_file = SD.open(full_path.c_str(), FILE_WRITE);
      for (int i = 0; i < inference.n_samples; i++)
      {
        sample_file.write(lowByte(inference.buffers[inference.buf_select][i]));
        sample_file.write(highByte(inference.buffers[inference.buf_select][i]));
      }
      sample_file.close();
      samples_recorded = samples_recorded + inference.n_samples;
    }

    sample_max = 0;
  
}

/**
 * @brief      Printf function uses vsnprintf and output using Arduino Serial
 *
 * @param[in]  format     Variable argument list
 */
void ei_printf(const char *format, ...)
{
  static char print_buf[1024] = {0};

  va_list args;
  va_start(args, format);
  int r = vsnprintf(print_buf, sizeof(print_buf), format, args);
  va_end(args);
#ifndef SERIAL_SOUND
  if (r > 0)
  {
    Serial.write(print_buf);
  }
#endif
  Serial.flush();
}

/**
 * @brief      PDM buffer full callback
 *             Get data and call audio thread callback
 */
static void pdm_data_ready_inference_callback(void)
{
  int bytesAvailable = PDM.available();

  if (EI_CLASSIFIER_FREQUENCY == 16000)
  {
    // read into the sample buffer
    int bytesRead = PDM.read((char *)&sampleBuffer[0], bytesAvailable);

    if (inference.buf_ready == 0)
    {
      for (int i = 0; i < bytesRead >> 1; i++)
      {
        inference.buffers[inference.buf_select][inference.buf_count++] = sampleBuffer[i];

        if (inference.buf_count >= inference.n_samples)
        {
          inference.buf_select ^= 1;
          inference.buf_count = 0;
          inference.buf_ready = 1;
          break;
        }
      }
    }
  }
  else if (EI_CLASSIFIER_FREQUENCY == 8000)
  {
    // read into the sample buffer
    int bytesRead = PDM.read((char *)&sampleBuffer[0], bytesAvailable);

    if (inference.buf_ready == 0)
    {
      for (int i = 0; i < bytesRead >> 1; i = i + 2)
      {

        inference.buffers[inference.buf_select][inference.buf_count++] = (sampleBuffer[i] / 2) + (sampleBuffer[i + 1] / 2);

        if (inference.buf_count >= inference.n_samples)
        {
          inference.buf_select ^= 1;
          inference.buf_count = 0;
          inference.buf_ready = 1;
          break;
        }
      }
    }
  }
  else
  {
    ei_printf(
        "ERROR - Classifier frequency not supported\n");
  }
}

/**
 * @brief      Init inferencing struct and setup/start PDM
 *
 * @param[in]  n_samples  The n samples
 *
 * @return     { description_of_the_return_value }
 */
static bool microphone_inference_start(uint32_t n_samples)
{

  inference.buffers[0] = (signed short *)malloc(n_samples * sizeof(signed short));

  if (inference.buffers[0] == NULL)
  {

    return false;
  }

  inference.buffers[1] = (signed short *)malloc(n_samples * sizeof(signed short));

  if (inference.buffers[1] == NULL)
  {
    free(inference.buffers[0]);
    return false;
  }
  float sample_length_sec = (float)n_samples / (float)EI_CLASSIFIER_FREQUENCY;
  int microphone_sample_count = int(16000 * sample_length_sec);
  ei_printf("Sample Length: %f sec, Mic Samp Count: %d", sample_length_sec, microphone_sample_count);
  sampleBuffer = (signed short *)malloc((microphone_sample_count >> 1) * sizeof(signed short));
  if (sampleBuffer == NULL)
  {
    free(inference.buffers[0]);
    free(inference.buffers[1]);
    return false;
  }
  inference.buf_select = 0;
  inference.buf_count = 0;
  inference.n_samples = n_samples;
  inference.buf_ready = 0;

  // configure the data receive callback
  PDM.onReceive(&pdm_data_ready_inference_callback);

  //ei_printf("Sector size: %d nblocks: %d\r\n", ei_nano_fs_get_block_size(), n_sample_blocks);
  PDM.setBufferSize((n_samples >> 1) * sizeof(int16_t));
  ei_printf("PDM Sample buffer set to: %d", (n_samples >> 1) * sizeof(int16_t));
  // initialize PDM with:
  // - one channel (mono mode)
  // - a 16 kHz sample rate
  if (!PDM.begin(1, 16000))
  { //EI_CLASSIFIER_FREQUENCY)) {
    ei_printf("Failed to start PDM!");
  }
  // optionally set the gain, defaults to 20
  PDM.setGain(80);


  return true;
}

/**
 * @brief      Wait on new data
 *
 * @return     True when finished
 */
static bool microphone_inference_record(void)
{
  inference.buf_ready = 0;
  inference.buf_count = 0;

  while (inference.buf_ready == 0)
  {
    delay(10);
  }

  for (long i = 0; i < inference.n_samples; i++)
  {
    if (inference.buffers[inference.buf_select][i] > sample_max)
    {
      sample_max = inference.buffers[inference.buf_select][i];
    }

  }

  return true;
}


/**
 * @brief      Stop PDM and release buffers
 */
static void microphone_inference_end(void)
{
  PDM.end();
  free(inference.buffers);
}
