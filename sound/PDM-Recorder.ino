/*
  This example reads audio data from the on-board PDM microphones, and prints
  out the samples to the Serial console. The Serial Plotter built into the
  Arduino IDE can be used to plot the audio data (Tools -> Serial Plotter)

  Circuit:
  - Arduino Nano 33 BLE board

  This example code is in the public domain.
*/

#include <PDM.h>
#include <hal/nrf_pdm.h>
#define EI_CLASSIFIER_FREQUENCY                  16000
#define RECORDING_SEC                            5
#define EI_CLASSIFIER_RAW_SAMPLE_COUNT           (EI_CLASSIFIER_FREQUENCY * RECORDING_SEC)
#define EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME      1
#define EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE       (EI_CLASSIFIER_RAW_SAMPLE_COUNT * EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME)
#define EI_CLASSIFIER_INTERVAL_MS                0.0625

const int ledPin = 22;
const int ledPin2 = 23;
const int ledPin3 = 24;


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



void setup() {
    pinMode(22, OUTPUT);
    pinMode(23, OUTPUT);
    pinMode(24, OUTPUT);
  Serial.begin(115200); //115200);
  while (!Serial);

  // initialize PDM with:
  // - one channel (mono mode)
  // - a 16 kHz sample rate
    if (microphone_inference_start(EI_CLASSIFIER_RAW_SAMPLE_COUNT) == false) {
        ei_printf("ERR: Failed to setup audio sampling\r\n");
        return;
    }
}




void loop() {
  byte start_mark[8] = {0,255,0,255,0,255,0,255};
  byte end_mark[8] = {0,255,0,255,255,0,255,0};
       digitalWrite(ledPin, LOW);
     digitalWrite(ledPin2, LOW);
     digitalWrite(ledPin3, LOW);

  bool m = microphone_inference_record();
    if (!m) {
        ei_printf("ERR: Failed to record audio...\n");
        return;
    }
     digitalWrite(ledPin, LOW);
     digitalWrite(ledPin2, HIGH);
     digitalWrite(ledPin3, HIGH);

  Serial.write(start_mark,8);
  for (int i=0; i < EI_CLASSIFIER_RAW_SAMPLE_COUNT ; i++) {
    Serial.write(lowByte(inference.buffer[i]*5));  
    Serial.write(highByte(inference.buffer[i]*5));
  }
  Serial.write(end_mark,8);
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

    if (r > 0) {
        Serial.write(print_buf);
    }
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

    if (inference.buf_ready == 0) {
        for(int i = 0; i < bytesRead>>1; i++) {
            inference.buffer[inference.buf_count++] = sampleBuffer[i];
            if(inference.buf_count >= inference.n_samples) {
                inference.buf_count = 0;
                inference.buf_ready = 1;
                break;
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
    byte lgain;
    byte rgain;
    // configure the data receive callback
    PDM.onReceive(&pdm_data_ready_inference_callback);

    //nrf_pdm_gain_get  ( &lgain,&rgain ); 
    //Serial.println(lgain);
    // optionally set the gain, defaults to 20
    PDM.setGain(80);
    //nrf_pdm_gain_get  ( &lgain,&rgain ); 
    //Serial.println(lgain);
    
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
