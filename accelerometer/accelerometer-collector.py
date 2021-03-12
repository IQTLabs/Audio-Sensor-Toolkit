"""Small utility that collects accelerometer samples from serial

These tools are designed to work with the LSM9DS1 accelerometer that is on the Arduino Nano 33 Sense. They show how to capture sensor reading at a high data rate, store them in a buffer and then send them over serial. It should be easy to port these tools to other sensors.

Args:
--label label for data
--dev serial device
"""

import json
import os, time, hmac, hashlib
import requests
import serial
import argparse
import errno
from datetime import datetime


# Parses the command line arguments
parser = argparse.ArgumentParser()
parser.add_argument('--label', type=str, help='label for data', required=True)
parser.add_argument('--dev', type=str, help='serial device', required=True)
args = parser.parse_args()

# Get your HMAC key from Edge Impulse by looking at the Keys tab of a project
HMAC_KEY = "<Get from Edge Impulse>"

# Connects to the serial device. 
ser = serial.Serial(args.dev,115200)

# empty signature (all zeros). HS256 gives 32 byte signature, and we encode in hex, so we need 64 characters here
emptySignature = ''.join(['0'] * 64)

# Adjust the length of the sample and the sample rate to match what is in the Arduino program
sample_sec = 6
sample_hz = 474

# settings are calculated
sample_interval_ms = (1/sample_hz) * 1000
samples_required = sample_sec * sample_hz
samples_recorded = 0
samples = []

# 'start' is sent over to the Arduino program, signalling it to start sampling
ser.write(b'start\n') 

# Loops until the expected numbers of samples have been collected
while samples_recorded < samples_required:
    try:
        # reads in a line of text off serial
        ser_bytes = ser.readline()

        # splits the line of text into 3 separate, tab delimanated values and then converts them to floats
        sample = [float(x) for x in str(ser_bytes,'ascii').split("\t")]

        # Checks to make sure it actually got three values, and then adds them to the totals
        if len(sample) == 3:
            samples.append(sample)
            samples_recorded = samples_recorded + 1
        else:
            print("Short sample: {}".format(len(sample)))
        
    except:
        break

# we have gotten all of the expected samples, so close the serial port
ser.close()

# Setup an object to store the samples in, using the CBOR format. The signature is the correct length, but is
# empty. The signature will be calculated after the message has been encoded, based on the final length.
data = {
    "protected": {
        "ver": "v1",
        "alg": "HS256",
        "iat": time.time() # epoch time, seconds since 1970
    },
    "signature": emptySignature,
    "payload": {
        "device_name": "ac:87:a3:0a:2d:1b",
        "device_type": "DISCO-L475VG-IOT01A",
        "interval_ms": sample_interval_ms,
        "sensors": [
            { "name": "accX", "units": "m/s2" },
            { "name": "accY", "units": "m/s2" },
            { "name": "accZ", "units": "m/s2" }
        ],
        "values": samples
    }
}

# encode in CBOR
encoded = json.dumps(data)

# sign message
signature = hmac.new(bytes(HMAC_KEY, 'utf-8'), msg = encoded.encode('utf-8'), digestmod = hashlib.sha256).hexdigest()

# set the signature again in the message, and encode again
data['signature'] = signature
encoded = json.dumps(data)

mydir = "./capture/" + args.label # the directory to store the JSON file in
try:
    os.makedirs(mydir) # make the dir structure
except OSError as e:
    if e.errno != errno.EEXIST: # not a big deal, the dir may already exist
        raise  # This was not a "directory exist" error..

# set a unique filename        
filename = "{}/{}_{}.json".format(mydir,args.label,datetime.now().strftime('%Y-%m-%d-%H-%M-%S'))
    
with open(filename, 'w') as outfile:
    outfile.write(encoded)