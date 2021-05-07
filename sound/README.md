# Sound TinyML Tools

These tools are design to work with the pulse density modulation (PDM) microphones on the [Arduino Nano 33 Sense](https://store.arduino.cc/usa/nano-33-ble-sense) and the [Adafruit Bluefruit Sense](https://learn.adafruit.com/adafruit-feather-sense). Both of these boards use the NordicRF 52840 chip, which internally handles the interface with a PDM microphone and converts output into pulse code modulation (PCM) encoded audio. The [Arduino PDM library](https://www.arduino.cc/en/Reference/PDM) wraps this all up nicely.

## Sample Collection

It is helpful to record samples directly with the onboard microphone to determine what the microphone is picking up. This will let you adjust the microphone's gain, which is especially helpful if you have placed the board inside a case.

#### audio_recorder.ino
This Arduino program let's you capture audio using the PDM microphone on the Adafruit Bluefruit Sense and save it to a MicroSD card. You can label the recordings as being either Sound or Noise. This will be reflected in the filename. The code was written to work with the:
- [Adafruit Bluefruit Sense](https://learn.adafruit.com/adafruit-feather-sense)
- Adafruit Adalogger Feather
- OLED Feather
- Adafruit Feather Tripler

To make a recording, hold down the A or B button on the OLED Feather. To stop the recording, hold down the C button. A .raw file will be saved on the microSD card. You can use the **raw-sample-to-wav.py** program to convert it to a .wav file.

#### PDM-Recorder.ino
This is a small Arduino program that will record at a given frequency rate for a specified time. The recorded audio will be saved to onboard memory. When the recording is done, it will transfer it over serial to the python program. One pattern of bytes marks the start of a recording and another the end. This helps the Python program figure out when to start recording and when a transfer is complete.

#### wav-collect.py
This Python 3 program receives a series of PCM audio samples over serial and writes it to a wav file. The following arguments need to be provided:
- `--label` This is the dataset label for the audio being collected. The files are named and saved into different subdirectories based on this label.
- `--dev` This is the path to the serial device for the Arduino board.

Here is an example of how to run this command:
````
python3 wav-collector.py --label=noise --dev=/dev/cu.usbmodem14201
````

The program will start, wait for the start pattern of bytes over serial and then capture the following bytes of PCM audio samples. Once the program detects the end pattern of bytes, it will save a .wav file. The program places the files into the *audio-capture* directory.

## Inference Collection

If you are trying to improve the accuracy of a category, it can be helpful to collect audio samples that classified as that category. This lets you relabel any false positives and retrain the model. 

#### inference-sound-collect.ino
This is an Arduino program that adds additional functionality to Edge Impulse's example program for sound classification. The different features are enabled using C macros, which can be uncommented at the top of the program:

- **OLED** If you are using the Adafruit board, you can add the [OLED Featherwing](https://learn.adafruit.com/adafruit-oled-featherwing?view=all). The program will print out the predictions each time inference is run.
- **SERIAL_SOUND** If the prediction for a specified class is over a certain amount, the collected sample will be sent over serial. The class being monitored and confidence level should be modified for your use case on line 290 of 'sound/inference-sound-collect.ino'. A Python program is used to collect the samples and save them. The arguments are the same as the **wav-collect.py** program above:
````
python3 wav-collector.py --label=noise --dev=/dev/cu.usbmodem14201
````
- **SD_SAVE** If you are using the Adafruit board, you can add the [Adalogger Featherwing](https://learn.adafruit.com/adafruit-adalogger-featherwing) and save collected samples to an SD card. If the prediction for a specified class is over a certain amount, the collected sample will be saved to the SD card. The class being monitored and confidence level should be modified for your use case on line 299 of 'sound/inference-sound-collect.ino'. The files are raw PCM samples and need to be converted into .wav files. The `sound/raw-sample-to-wav.py` program does this conversion. Use the `--input` argument to provide the path to where these files are being stored. The converted .wav file will be created alongside them:
````
python3 raw-sample-to-wav.py --input=capture-7-20
````
