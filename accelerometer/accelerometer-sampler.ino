#include <Arduino_LSM9DS1.h>

#define CONVERT_G_TO_MS2    9.80665f
#define ACCEL_HZ 474
#define SAMPLE_SEC 6
#define SAMPLE_COUNT (ACCEL_HZ * SAMPLE_SEC)
#define VALUES_PER_SAMPLE 3
#define SAMPLE_BUFFER_SIZE (SAMPLE_COUNT * VALUES_PER_SAMPLE)


// Generally, you should use "unsigned long" for variables that hold time
// The value will quickly become too large for an int to store

bool sample_mode=false;
bool new_data=false;
const byte numChars = 32;
char receivedChars[numChars]; // an array to store the received data
float buffer[SAMPLE_COUNT][VALUES_PER_SAMPLE];
int sample_count = 0;

// constants won't change:
const long interval = 10;           // interval at which to blink (milliseconds)

void setup() {
    Serial.begin(115200);
    while (!Serial);
    //Serial.println("Started");

    if (!IMU.begin()) {
        Serial.println("Failed to initialize IMU!");
        while (1);
    }
       // note the FS value does not change the output of the read method, it just assigns more bits to the chip output,
   // increasing accuracy at the cost of a smaller range 
   IMU.setAccelFS(0);   // 0: ±2g ; 1: ±24g ; 2: ±4g ; 3: ±8g  (default =2)
   //Serial.print("Accelerometer Full Scale = ±");
   //Serial.print(IMU.getAccelFS());
   //Serial.println ("g");

   // Change the sample frequency ( ODR = Output Dats rate)  
   IMU.setAccelODR(5);  // Output Data Rate 0:off, 1:10Hz, 2:50Hz, default = 3:119Hz, 4:238Hz, 5:476Hz, (not working 6:952Hz) 
   //Serial.print("Accelerometer sample rate = ");
   //Serial.print(IMU.getAccelODR());         // alias  AccelerationSampleRate());
   //Serial.println(" Hz \n");

   //Serial.println("X\tY\tZ");
}

void recvWithEndMarker() {
 static byte ndx = 0;
 char endMarker = '\n';
 char rc;
 
 // if (Serial.available() > 0) {
           while (Serial.available() > 0 && new_data == false) {
 rc = Serial.read();

 if (rc != endMarker) {
 receivedChars[ndx] = rc;
 ndx++;
 if (ndx >= numChars) {
 ndx = numChars - 1;
 }
 }
 else {
 receivedChars[ndx] = '\0'; // terminate the string
 ndx = 0;
 new_data = true;
 }
 }
}
void checkInput() {
 if (new_data == true) {
 sample_mode = true;
 sample_count = 0;
 new_data = false;
 }
}

void loop() {
  
  if (sample_mode) {
    float x, y, z;
    
    if (sample_count< SAMPLE_COUNT) {
      if (IMU.accelerationAvailable()) {
        IMU.readAcceleration(x, y, z);
    
        buffer[sample_count][0] = x * CONVERT_G_TO_MS2;
        buffer[sample_count][1] = y * CONVERT_G_TO_MS2;
        buffer[sample_count][2] = z * CONVERT_G_TO_MS2;
        sample_count++;
      }
    } else {
      for (int i=0; i < SAMPLE_COUNT; i++) {
          Serial.print(buffer[i][0]);
          Serial.print('\t');
          Serial.print(buffer[i][1]);
          Serial.print('\t');
          Serial.println(buffer[i][2]);
      }
      //Serial.println("END");
      sample_mode = false;
    }
  } else {
    recvWithEndMarker();
    checkInput();
  }
}
