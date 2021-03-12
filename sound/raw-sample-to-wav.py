""" A small utility that converts raw pcm audio into wav files

Converts raw PCM samples into .wav files. The .wav files will be stored alongside the .raw files
`--input` the path to where the raw files are being stored.
"""


import json
import os, time, hmac, hashlib
import requests
import serial
import argparse
import errno
import wave
import audioop
import struct
from datetime import datetime
from pydub import AudioSegment, effects  

# Parses command line arguments
parser = argparse.ArgumentParser()
parser.add_argument('--input', type=str, help='path to the .raw files', required=True)
args = parser.parse_args()

if os.path.exists(args.input):
    print("Converting raw files in: {}".format(os.path.abspath(args.input)))
else:
    print("Input path does not exist")
    os.exit()

filepath = os.path.abspath(args.input)

# These values should be adjusted to match the sample length and sample rate used in the Arduino program
sample_sec = 10
sample_hz = 16000

samples_required = sample_sec * sample_hz
bytes_required = samples_required * 2

directory = os.fsencode(args.input)
    
# iterate through all of the files in the directory
for file in os.listdir(directory):

    filename = os.fsdecode(file)
    if os.path.isfile(filepath + "/" + filename):   # check to make sure it is file and not a directory
        
        f=open(filepath + "/" + filename,"rb")

        raw_audio = AudioSegment.from_file(filepath + "/" +filename, format="raw",         # read the audio samples from the file 
                                    frame_rate=sample_hz, channels=1, sample_width=2)
        
        pre, ext = os.path.splitext(filename) # create the .wav filename
        wav_filename = pre+".wav"

        #normalizedsound = effects.normalize(raw_audio)  # normalize the audio volume

        ##normalizedsound.export(filepath + "/" + wav_filename, format="wav")  # save the .wav file
        raw_audio.export(filepath + "/" + wav_filename, format="wav")  # save the .wav file