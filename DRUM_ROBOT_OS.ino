#include <Servo.h>

//Velocity≈0.1960​≈315.79 degrees/second

//IT takes minimum 1600 ms per motion

Servo servo1;  // servo1 object representing the MG 996R servo1
Servo servo2;
Servo servo3;

int bpm = 60; //15;
unsigned long beat_duration = (60UL * 1000UL) / bpm;


int volume = 50;  //min volume = 17 for now, MAX velocity 90
char button_state = 'o';
int hit_angle_1 = 38;
const int hit_angle_2 = 0;


const unsigned long tf_min = 350;   // Fastest movement for highest volume (100%)
const unsigned long tf_max = 800;  // Slowest movement for lowest volume (0%)

const int min_init_angle_1 = 38;
const int min_init_angle_2 = 0;
const int max_init_angle_1 = 120;
const int max_init_angle_2 = 50;

void setup() {
  // Initialize serial communication
  Serial.begin(9600);
  servo1.attach(3);  // servo1 is wired to Arduino on digital pin 3
  servo2.attach(4);
  servo3.attach(5);
  move_UP(40, 0, 100);
}


int computeElbowAngle_cubic(int initialAngle, int finalAngle, unsigned long movementTime, unsigned long startTime, unsigned long currentTime) {
  float t = (float)(currentTime - startTime) / movementTime;

  if (t >= 1.0) {
    return finalAngle;
  } else if (t <= 0.0) {
    return initialAngle;
  }

  float angleRange = finalAngle - initialAngle;

  // Using the cubic polynomial equation: θ(t) = θ0 + (θf - θ0) * (3t^2 - 2t^3)
  float currentAngle = initialAngle + angleRange * (3 * t * t - 2 * t * t * t);

  return (int)currentAngle;
}


int computeWristAngle_quartic(int initialAngle, int finalAngle, unsigned long movementTime, unsigned long startTime, unsigned long currentTime) {
  float t = (float)(currentTime - startTime) / movementTime;

  if (t >= 1.0) {
    return finalAngle;
  } else if (t <= 0.0) {
    return initialAngle;
  }

  float angleRange = finalAngle - initialAngle;

  // Using the quartic polynomial equation to simulate the desired acceleration profile
  // θ(t) = θ0 + (θf - θ0) * (10t^3 - 15t^4 + 6t^5)
  float currentAngle = initialAngle + angleRange * (10 * t * t * t - 15 * t * t * t * t + 6 * t * t * t * t * t);

  return (int)currentAngle;
}


void move_UP(double start_angle_1, double start_angle_2, double resting_angle_3) {
  //Serial.print("moving servos UP\n");

  servo1.write((int)start_angle_1);
  servo2.write((int)start_angle_2);
  servo3.write((int)resting_angle_3);

  unsigned long time = millis();
  while (millis() - time <= 1000) {
    // wait for the servos to go up
  }
  //Serial.print("Completed movement\n");
}

void move_DOWN(double start_angle_1, double start_angle_2, double hit_angle_1, double hit_angle_2, double hit_angle_3, unsigned long tf, char buttonState) {
  //Serial.print("moving servos DOWN\n");
  unsigned long action_time = millis();
  unsigned long delay_time = tf / 4;

  while (millis() - action_time <= tf) {  // until the action is finished
    int theta1 = computeElbowAngle_cubic((int)start_angle_1, (int)hit_angle_1, tf, action_time, millis());

    //Serial.print(theta1);
    //Serial.print(" = theta 1\n");

    servo1.write(theta1);

    if((int)hit_angle_1 - theta1 < 10 ){ // bypass the delay when volume is very lo or when we start close
      servo3.write((int)hit_angle_3);
      delay_time = tf / 10;
    }

    if ((millis() - action_time) > delay_time){  // wait half a second before moving servo 3

      int theta2 = computeWristAngle_quartic((int)start_angle_2 + 20, (int)hit_angle_2, tf - delay_time, action_time + delay_time, millis());
      //Serial.print(theta2);
      //Serial.print(" = theta 2\n");

      servo2.write(theta2);

      if (((millis() - action_time) <= tf - 100) && (buttonState == 'o')) {
        servo3.write(118);
      } else {
        servo3.write((int)hit_angle_3);
      }


      //Serial.print("moving servo 3\n");
    } else {  //give a bit more back angle to the wrist
      int theta2 = computeElbowAngle_cubic((int)start_angle_2, (int)start_angle_2 + 20, delay_time, action_time, millis());
      servo2.write(theta2);
    }
  }
  //Serial.print("Completed movement\n");
}

unsigned long volumeToTf(int volume_) {
  // Ensure volume_ is within the range 0 to 100
  volume_ = constrain(volume_, 0, 100);

  // Map volume_ to trajectory time (inverse relationship)
  return map(volume_, 0, 100, tf_max, tf_min);
}



void loop() {
  static unsigned long lastSerialReadTime = 0;
  static String data = "";
  static int state = 0;  // Initialize state variable

  // Non-blocking check for serial data every 10ms
  if (millis() - lastSerialReadTime > 10) {
    lastSerialReadTime = millis();
    while (Serial.available() > 0) {
      char ch = (char)Serial.read();
      if (ch == '\n') {
        // Parse the data
        bpm = data.substring(0, data.indexOf(',')).toInt();  // BPM
        data.remove(0, data.indexOf(',') + 1);
        volume = data.substring(0, data.indexOf(',')).toInt();  // percentage of volume we want
        data.remove(0, data.indexOf(',') + 1);
        button_state = data.charAt(0);
        data.remove(0, data.indexOf(',') + 1);
        state = data.toInt();  // Read the state variable

        beat_duration = (60UL * 1000UL) / bpm;

        data = "";  // Clear the buffer
      } else {
        data += ch;
      }
    }
  }

  double alpha1 = 0.5;
  double alpha2 = 1;

  double start_angle_1 = min_init_angle_1 + alpha1 * (max_init_angle_1 - min_init_angle_1) * (volume / 100.0);
  double start_angle_2 = min_init_angle_2 + alpha2 * (max_init_angle_2 - min_init_angle_2) * (volume / 100.0);

  double resting_angle_3 = 118;
  double hit_angle_3 = resting_angle_3;
  //------------------------------------------------ACTION CYCLE----------------------------------------------------//

  //----------------------------------------------------------------------------------------------------------------//

  static unsigned long action_time = millis();  //initial time of the beat/action
  static int flag = 0;


  if (flag == 0 && state == 1) {  //if the state is to move and if we did not complete the action
    if (button_state != 'm') {    // open or rolled beat
      hit_angle_3 = 100;
      //Serial.print("OPEN OR MUTED BEAT\n");
    }

    if (button_state == 'r') {
      hit_angle_1 = 42;
    } else {
      hit_angle_1 = 38;  //rolled beat
    }

    move_DOWN(start_angle_1, start_angle_2, hit_angle_1, hit_angle_2, hit_angle_3, volumeToTf(volume), button_state);

    if (button_state == 'r') {  // rolled beat
      unsigned long currTime = millis();
      unsigned long time_left = beat_duration - (currTime - action_time);
      unsigned long margin = 400;
      while (millis() - currTime < time_left - margin) {
        //currTime - action_time = time spent for the action
        // beat_duration - (currTime - action_tim) = time left for the action
        // time left for the action - margin (=400)
        //          WAIT
        //Serial.print("ROLLED BEAT\n");
      }
    }

    move_UP(start_angle_1, start_angle_2, 118);
    flag = 1;
    //Serial.print("Finished moving servos\n");
  }

  //----------------------------------------------------------------------------------------------------------------//
}