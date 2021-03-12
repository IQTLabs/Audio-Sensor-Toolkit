# Audio Sensor Toolkit

This is a guide on how to build an Audio Sensor using Machine Learning, and helpful tools.

### [Audio Sensor Guide](./guide/overview.md)
### [Audio Tools](./sound/README.md)
### [Accelerometer Tools](./accelerometer/README.md)

The tools are a set of scripts to help capture and process sensor data coming from microcontrollers running Arduino. "TinyML" is the catch phrase for running optimized [TensorFlow](https://www.tensorflow.org/) models on microcontrollers. These tools have been written to work with [Edge Impulse](https://www.edgeimpulse.com/), a platform for developing ML models for Edge devices. These scripts can be easily adapted if you are working directly with [TensorFlow Lite micro](https://www.tensorflow.org/lite/microcontrollers).

The tools record data from the sensors on-board the Arduino development boards and transfer the data to a computer over a serial connection. Since the sampling rates are often higher than the speed of the serial connection, the program buffers the sensor data in memory and then transfers it. Some of the tools also support saving the collected samples to an optionally attached MicroSD card.

Two different types of sensor data are supported:
- **PDM data** from a microphone. These tools are located in the *sound* folder.
- **3 Axis Accelerometer data** from the LSM9DS1 sensor, which is on the [Arduino Nano 33 Sense](https://store.arduino.cc/usa/nano-33-ble-sense). These tools are located in the *accelerometer* folder.

## Install the requirements
To install the required modules, run:
````
pip3 install -r requirements.txt
````

## Uploading to Edge Impulse
The collected samples can be uploaded to the Edge Impulse platform using the [upload tool](https://docs.edgeimpulse.com/docs/cli-uploader). You can reset what project you are uploading to using the --clean argument:


````
edge-impulse-uploader --clean
````

Here is an example of uploading some noise samples:

````
edge-impulse-uploader --label noise --category testing *.wav
````
