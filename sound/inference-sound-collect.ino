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
 * all copies or substantial portions of the Software.
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
#define EIDSP_QUANTIZE_FILTERBANK   0
//#define EIDSP_USE_CMSIS_DSP 0

/* Includes ---------------------------------------------------------------- */
#include <PDM.h>

// Include the appropriate Edge Impulse library here:
#include <insert_edge_impulse_inference_library>

#include <edge-impulse-sdk/dsp/numpy.hpp>
#include <SPI.h>

#include <Wire.h>



   
#define OLED true
//#define SERIAL_SOUND true
#define SD_SOUND true


#ifdef SD_SOUND
// Set the pins used
//#include <SD.h>
#include "SdFat.h"
SdFat SD;
#define cardSelect 10
#endif

#ifdef OLED
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
Adafruit_SSD1306 display = Adafruit_SSD1306(128, 32, &Wire);

// OLED FeatherWing buttons map to different pins depending on board:
// 32u4, M0, M4, nrf52840 and 328p
//  #define BUTTON_A  9
//  #define BUTTON_B  6
//  #define BUTTON_C  5
#endif


/* Battery stuff */
uint32_t vbat_pin = PIN_VBAT;             // A7 for feather nRF52832, A6 for nRF52840

#define VBAT_MV_PER_LSB   (0.73242188F)   // 3.0V ADC range and 12-bit ADC resolution = 3000mV/4096

#ifdef NRF52840_XXAA
#define VBAT_DIVIDER      (0.5F)          // 150K + 150K voltage divider on VBAT
#define VBAT_DIVIDER_COMP (2.0F)          // Compensation factor for the VBAT divider
#else
#define VBAT_DIVIDER      (0.71275837F)   // 2M + 0.806M voltage divider on VBAT = (2M / (0.806M + 2M))
#define VBAT_DIVIDER_COMP (1.403F)        // Compensation factor for the VBAT divider
#endif

#define REAL_VBAT_MV_PER_LSB (VBAT_DIVIDER_COMP * VBAT_MV_PER_LSB)



int counter=0;


/* From https://github.com/adafruit/Adafruit_nRF52_Arduino/blob/master/libraries/Bluefruit52Lib/examples/Hardware/adc_vbat/adc_vbat.ino */

float readVBAT(void) {
  float raw;

  // Set the analog reference to 3.0V (default = 3.6V)
  analogReference(AR_INTERNAL_3_0);

  // Set the resolution to 12-bit (0..4095)
  analogReadResolution(12); // Can be 8, 10, 12 or 14

  // Let the ADC settle
  delay(1);

  // Get the raw 12-bit, 0..3000mV ADC value
  raw = analogRead(vbat_pin);

  // Set the ADC back to the default settings
  analogReference(AR_DEFAULT);
  analogReadResolution(10);

  // Convert the raw value to compensated mv, taking the resistor-
  // divider into account (providing the actual LIPO voltage)
  // ADC range is 0..3000mV and resolution is 12-bit (0..4095)
  return raw * REAL_VBAT_MV_PER_LSB;
}

uint8_t mvToPercent(float mvolts) {
  if(mvolts<3300)
    return 0;

  if(mvolts <3600) {
    mvolts -= 3300;
    return mvolts/30;
  }

  mvolts -= 3600;
  return 10 + (mvolts * 0.15F );  // thats mvolts /6.66666666
}


void display_error(String text) {
  #ifdef OLED
  display.clearDisplay();
  display.display();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.print("Error!");
  display.print(text);
  display.setCursor(0,0);
  display.display();
  #endif
}


/** Audio buffers, pointers and selectors */
typedef struct {
    int16_t *buffer;
    uint8_t buf_ready;
    uint32_t buf_count;
    uint32_t n_samples;
} inference_t;

static inference_t inference;
static bool record_ready = false;
static signed short sampleBuffer[2048];
static bool debug_nn = false; // Set this to true to see e.g. features generated from the raw signal
static int samples_saved = 0;

/**
 * @brief      Arduino setup function
 */
void setup()
{

      Serial.begin(115200);

   delay(100);
    #ifdef SD_SOUND
      // see if the card is present and can be initialized:
      if (!SD.begin(cardSelect)) {
        Serial.println("Card init. failed!");
        while(1);
      }
      File count_file;
      count_file = SD.open("samples_saved.txt");
      if (count_file.available()) {
        samples_saved = count_file.parseInt();
      }
      count_file.close();
      Serial.print("Starting files at: ");
      Serial.println(samples_saved);
    #endif


   #ifdef OLED
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C); // Address 0x3C for 128x32
    // Show image buffer on the display hardware.
    // Since the buffer is intialized with an Adafruit splashscreen
    // internally, this will display the splashscreen.
    display.display();
    //pinMode(BUTTON_A, INPUT_PULLUP);
    //pinMode(BUTTON_B, INPUT_PULLUP);
    //pinMode(BUTTON_C, INPUT_PULLUP);
  #endif

  #ifdef OLED
    char line[100];
    display.clearDisplay();
    display.display();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0,0);
    sprintf(line, "Hello World\n");
    display.print(line);
    display.setCursor(0,0);
    display.display();
  #endif


    // summary of inferencing settings (from model_metadata.h)
    ei_printf("Inferencing settings:\n");
    ei_printf("\tInterval: %.2f ms.\n", (float)EI_CLASSIFIER_INTERVAL_MS);
    ei_printf("\tFrame size: %d\n", EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE);
    ei_printf("\tSample length: %d ms.\n", EI_CLASSIFIER_RAW_SAMPLE_COUNT / 16);
    ei_printf("\tNo. of classes: %d\n", sizeof(ei_classifier_inferencing_categories) / sizeof(ei_classifier_inferencing_categories[0]));

    if (microphone_inference_start(EI_CLASSIFIER_RAW_SAMPLE_COUNT) == false) {
        ei_printf("ERR: Failed to setup audio sampling\r\n");
        return;
    }
}




/**
 * @brief      Arduino main function. Runs the inferencing loop.
 */
void loop()
{
    byte start_mark[8] = {0,255,0,255,0,255,0,255};
    byte end_mark[8] = {0,255,0,255,255,0,255,0};
    char line[100];

    ei_printf("Starting inferencing in 2 seconds...\n");

    //delay(2000);

    ei_printf("Recording...\n");

    bool m = microphone_inference_record();
    if (!m) {
        ei_printf("ERR: Failed to record audio...\n");
        return;
    }

    ei_printf("Recording done\n");

    signal_t signal;
    signal.total_length = EI_CLASSIFIER_RAW_SAMPLE_COUNT;
    signal.get_data = &microphone_audio_signal_get_data;
    ei_impulse_result_t result = { 0 };

    EI_IMPULSE_ERROR r = run_classifier(&signal, &result, debug_nn);
    if (r != EI_IMPULSE_OK) {
        ei_printf("ERR: Failed to run classifier (%d)\n", r);
        return;
    }

    // print the predictions
    ei_printf("Predictions (DSP: %d ms., Classification: %d ms., Anomaly: %d ms.): \n",
        result.timing.dsp, result.timing.classification, result.timing.anomaly);
    for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
        ei_printf("    %s: %.5f\n", result.classification[ix].label, result.classification[ix].value);
    }

    int noise = (int) (result.classification[0].value * 100);
    int siren = (int) (result.classification[1].value * 100);
     #ifdef OLED
      // Get a raw ADC reading
      float vbat_mv = readVBAT();
    
      // Convert from raw mv to percentage (based on LIPO chemistry)
      uint8_t vbat_per = mvToPercent(vbat_mv);

        display.clearDisplay();
        display.setCursor(0,0);
        
        //sprintf(line, "DSP: %d ms.\nClass: %d ms.\n",result.timing.dsp, result.timing.classification);
        //display.print(line);
        sprintf(line, "Noise: %d\n",noise);
        display.print(line);
        sprintf(line, "Siren: %d\n",siren);
        display.print(line);
        sprintf(line, "mv: %f bat: %d\n",vbat_mv,vbat_per);
        display.print(line);
        display.display();
      #endif

      #ifdef SERIAL_SOUND

      if (result.classification[1].value > 0.50) {
        Serial.write(start_mark,8);
        for (int i=0; i < EI_CLASSIFIER_RAW_SAMPLE_COUNT ; i++) {
            Serial.write(lowByte(inference.buffer[i]));  
            Serial.write(highByte(inference.buffer[i]));
        }
        Serial.write(end_mark,8);
      }
      #endif
      #ifdef SD_SOUND
      if (result.classification[1].value > 0.50) {
      File count_file;
      File sample_file;
        String label = result.classification[1].label;
        String filename = label + samples_saved + ".raw";
        SD.mkdir(label.c_str());
        String full_path = label + "/" + filename;
        Serial.print("Saving sample to file: ");
        Serial.println(full_path);
        sample_file = SD.open(full_path.c_str(), FILE_WRITE);
        for (int i=0; i < EI_CLASSIFIER_RAW_SAMPLE_COUNT ; i++) {
            sample_file.write(lowByte(inference.buffer[i]));  
            sample_file.write(highByte(inference.buffer[i]));
        }
        sample_file.close();
        count_file = SD.open("samples_saved.txt", FILE_WRITE | O_TRUNC);
        samples_saved++;
        count_file.print(samples_saved);
      count_file.close();
      }
      #endif
#if EI_CLASSIFIER_HAS_ANOMALY == 1
    ei_printf("    anomaly score: %.3f\n", result.anomaly);
#endif
}

/**
 * @brief      Printf function uses vsnprintf and output using Arduino Serial
 *
 * @param[in]  format     Variable argument list
 */
void ei_printf(const char *format, ...) {
    static char print_buf[1024] = { 0 };

    va_list args;
    va_start(args, format);
    int r = vsnprintf(print_buf, sizeof(print_buf), format, args);
    va_end(args);
    #ifndef SERIAL_SOUND
    if (r > 0) {
        Serial.write(print_buf);
    }
    #endif
}


/**
 * @brief      PDM buffer full callback
 *             Get data and call audio thread callback
 */
static void pdm_data_ready_inference_callback(void)
{
    int bytesAvailable = PDM.available();

    // read into the sample buffer
    int bytesRead = PDM.read((char *)&sampleBuffer[0], bytesAvailable);

    //if (record_ready == true || inference.buf_ready == 1) {
    if (inference.buf_ready != 1) {
        for(int i = 0; i < bytesRead>>1; i++) {
            inference.buffer[inference.buf_count++] = sampleBuffer[i];

            if(inference.buf_count >= inference.n_samples) {
                inference.buf_count = 0;
                inference.buf_ready = 1;
            }
        }
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
    inference.buffer = (int16_t *)malloc(n_samples * sizeof(int16_t));

    if(inference.buffer == NULL) {
        return false;
    }

    inference.buf_count  = 0;
    inference.n_samples  = n_samples;
    inference.buf_ready  = 0;

    // configure the data receive callback
    PDM.onReceive(&pdm_data_ready_inference_callback);

    // optionally set the gain, defaults to 20
    PDM.setGain(80);

    //ei_printf("Sector size: %d nblocks: %d\r\n", ei_nano_fs_get_block_size(), n_sample_blocks);
    PDM.setBufferSize(4096);

    // initialize PDM with:
    // - one channel (mono mode)
    // - a 16 kHz sample rate
    if (!PDM.begin(1, EI_CLASSIFIER_FREQUENCY)) {
        ei_printf("Failed to start PDM!");
    }

    record_ready = true;

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

    while(inference.buf_ready == 0) {
        delay(10);
    }

    return true;
}

/**
 * Get raw audio signal data
 */
static int microphone_audio_signal_get_data(size_t offset, size_t length, float *out_ptr)
{
    //arm_q15_to_float(&inference.buffer[offset], out_ptr, length);

    int16_t *input = &inference.buffer[offset];
    for (size_t ix = 0; ix < length; ix++) {
            out_ptr[ix] = (float)(input[ix]);
    }
    return 0;
}

/**
 * @brief      Stop PDM and release buffers
 */
static void microphone_inference_end(void)
{
    PDM.end();
    free(inference.buffer);
}

#if !defined(EI_CLASSIFIER_SENSOR) || EI_CLASSIFIER_SENSOR != EI_CLASSIFIER_SENSOR_MICROPHONE
#error "Invalid model for current sensor."
#endif
