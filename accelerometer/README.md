# Accelerometer TinyML Tools

These tools are designed to work with the LSM9DS1 accelerometer that is on the Arduino Nano 33 Sense. They show how to capture sensor reading at a high data rate, store them in a buffer and then send them over serial. It *should* be easy to port these tools to other sensors.

## Requirements

These tools use the re-written Arduino library for the LSM9DS1: https://github.com/FemmeVerbeek/Arduino_LSM9DS1
This library adds support for changing the sample rate of the accelerometer. Follow the directions in the repo to install it.

## Accelerometer data sampling

The accelerometer captures data at a higher rate than can be sent over the serial port. In order to sample data you need to first buffer it onboard during the sampling period and then send it over serial.

#### accelerometer/accelerometer-sampler.ino
This is a small Arduino program. When it receives a start command (anything, followed by a newline) it will start buffering samples for the specified sample period and then send them back over serial. Change `SAMPLE_SEC` to adjust how many seconds should be recorded. The value should be the same in both the Arduino and Python programs. The sample rate for the accelerometer can be adjusted in *setup()* by changing *IMU.setAccelODR();*. The *ACCEL_HZ* value should be updated accordingly.

#### accelerometer/accelerometer-collector.py
Each time this Python program is run, it will have the attached Arduino device start sampling data and then wait for it to be sent over serial. This data will be converted into an array and saved as a JSON file in the CBOR format. More information on this format can be found at [Edge Impulse docs](https://docs.edgeimpulse.com/reference#data-acquisition-format). All of the files are expected to be signed. The program handles this automatically, but it needs to be configured with your HMAC key from Edge Impulse:
````
HMAC_KEY = "<Get from Edge Impulse>"
````
The program has the following arugments:
- '--label' This is the *label* you wish to assign to the data. It will be stored in a subdirectory based on the label.
- `--dev` This is the path to the serial device for the Arduino board.

````
python3 accelerometer-collector.py --label=fill --dev=/dev/cu.usbmodem14201
````

Both the sample period and sample rate should be updated to match whatever is in the Arduino program. These values are used to determine how many samples the program should wait for before creating the file.

````
sample_sec = 6
sample_hz = 474
````

## Inference

#### inference-example.ino
This is an updated version of an Edge Impulse Accelerometer inference example. It has been updated to allow for different sampling rates, to set the RGB LED color based based on prediction confidence, and to print the predictions as comma separated values over serial. These values can be collected to create a graph of predictions over time. Simply `cat`ing the serial device to a file works great: `cat /dev/cu.usbmodem14401 > test_run.csv`

Make sure to configure the sample rate to the same rate used for collection!
