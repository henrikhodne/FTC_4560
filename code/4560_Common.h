/**
 * Common functions for team 4560's TeleOp and Autonomous programs.
 * All code written by Henrik Hodne unless otherwise noted. All code by Henrik
 * Hodne is released under the MIT license (see the LICENSE file).
 */


#ifndef __4560_COMMON_H__
#define __4560_COMMON_H__

#include "HTMC-driver.h"

// Positions for the double servo on the scoop
#define scoopServoUp 156
#define scoopServoDown 31

// Positions for the servo with the compass arm
#define compassHolderUp 0
#define compassHolderDown 20

// Values for the TopHat positions
#define TopHat_Idle -1
#define TopHat_Up 0
#define TopHat_Down 4

float atan2(float xVal, float yVal)
{
  if (xVal > 0)
    return atan(yVal/xVal);
  else if (yVal >= 0 && xVal < 0)
    return PI + atan(yVal/xVal);
  else if (yVal < 0 && xVal > 0)
    return -PI + atan(yVal/xVal);
  else if (yVal > 0 && xVal == 0)
    return PI/2;
  else if (yVal < 0 && xVal == 0)
    return -PI/2;
  else
    return 0; // Actually undefined
}

void setMotors(int mNEvalue, int mNWvalue, int mSWvalue, int mSEvalue)
{
#ifdef CONNECTION_DETECTION
  if (kInFailureMode)
    return;
#endif
  motor[motorNE] = mNEvalue;
  motor[motorNW] = mNWvalue;
  motor[motorSW] = mSWvalue;
  motor[motorSE] = mSEvalue;
}

// True: Logarithmic scale. False: Linear scale.
bool bUseLogarithmicScale = true;
// Adjust to set max power level to be used.
const int kMaximumPowerLevel = 100;

/**
 * Scale joystick input (which goes from -128 to 127) to another scale (like
 * motors, which go from -100 to 100).
 *
 * @author Ian McBride
 * @param yOrig The number to scale (should be between -128 to 127).
 * @param nMaxPower The maximum level to scale it to (default is 100).
 * @return The scaled value, between -nMaxPower and nMaxPower.
 */
int scaleJoystick(int &yOrig, int nMaxPower = kMaximumPowerLevel)
{
  // Scaled x or y value
  int yScaled;

  // Instead of using floating point math or having a 128 element table we're
  // only going to define a small set of data points in the range.
  static const int nLogScale[33] =
  {
    0,    0,   6,   7,   8,   9,  10,  11,
    12,  14,  15,  17,  18,  22,  22,  24,
    30,  33,  36,  40,  43,  47,  50,  55,
    60,  66,  72,  77,  81,  89,  95, 100,
    100
  };

  // Ensure that the value we scale is in the range [-128, 127].
  if (yOrig < 0)
    yScaled = max(yOrig, -127);
  else
    yScaled = min(yOrig, 127);

  if (bUseLogarithmicScale)
  {
    // Scale the joystick value to the size of the nLogScale array.
    yScaled /= 4;

    if (yScaled >= 0)
      yScaled = nLogScale[yScaled];
    else
      yScaled = -nLogScale[-yScaled];
  }
  else if (abs(yScaled) < 10) // Dead band in linear scale
    yScaled = 0;
  else
  {
    yScaled *= 100;
    yScaled /= 127;
  }

  // Scale the results again if we are limiting the top end.
  // Note: Ignores bad parameter values.
  if (nMaxPower < kMaximumPowerLevel && nMaxPower > 0)
  {
    yScaled *= nMaxPower;
    yScaled /= kMaximumPowerLevel;
  }

  return yScaled;
}

/**
 * This sets up the compass and compass holder. Should be called by all
 * programs using the compass.
 */
void compassSetup()
{
  // This isn't implemented in ROBOTC yet, but when it is this will protect the
  // servo from trying to push the arm into a C-channel.
  //servoMinPos[servoCompass] = compassHolderUp;
  //servoMaxPos[servoCompass] = compassHolderDown;
}

/**
 * Rotate the compass holder to the "up" position.
 */
void compassUp()
{
  servo[servoCompass] = compassHolderUp;
}

/**
 * Rotate the compass holder to the "down" position.
 *//*
void compassDown()
{
  servo[servoCompass] = compassHolderDown;
}
*/
/**
 * Cap an integer to be between -100 and 100.
 *
 * If the value passed is larger than 100, it's set to 100. It it's smaller
 * than -100, it's set to -100.
 *
 * @param value The integer to cap.
 * @return A number between -100 and 100 (inclusive).
 */
int cap100(int value)
{
  if (value > 100)
    return 100;
  else if (value < -100)
    return -100;
  else
    return value;
}

/**
 * Moves the robot in a direction (given in degrees) where 0 degrees is "East".
 *
 * Hint: This function basically takes a velocity vector.
 *
 * @param speed The speed at which to move, or magnitude of vector.
 * @param angle The heading at which to move, or angle of vector (in degrees).
 */
void moveRobot(int speed, int angle)
{
  // Use cap100() to limit the joysticks to a circle, and then the robot won't
  // move faster when going at an angle (besides, the motors only go to 100).
  float x_value = cosDegrees(angle) * cap100(speed);
  float y_value = sinDegrees(angle) * cap100(speed);

  setMotors(
    -x_value + y_value, // NE
    -x_value - y_value, // NW
    x_value - y_value,  // SW
    x_value + y_value); // SE
}

/**
 * Spin around on the spot with a given speed. A positive speed rotates
 * counter-clockwise and negative clockwise. Setting speed to 0 stops the
 * robot.
 *
 * @param speed The speed at which to spin, between -100 and 100.
 */
void spin(int speed)
{
  setMotors(-speed, -speed, -speed, -speed);
}

/**
 * Spin until we're heading towards a given heading.
 *
 * @param heading The heading at which to point when done turning. 0˚ is N.
 * @param speed The speed at which to spin (lower is more accurate).
 * @return Whether we successfully turned.
 */
bool turnToHeading(const int heading)
{
  // To prevent us from getting stuck.
  int numReadings, direction, leftToTurn, speed;

  int lastAngle = HTMCreadHeading(sensorCompass);
  wait1Msec(50);

  do
  {
    leftToTurn = (heading - lastAngle) % 360;
    if (leftToTurn > 180)
    {
      direction = -1;
      leftToTurn -= 180;
    } else {
      direction = 1;
    }

    speed = 12 + (leftToTurn/10.0);

    spin(speed*direction);
    wait10Msec(1); // To give it a chance to start moving.

    lastAngle = HTMCreadHeading(sensorCompass);
    wait1Msec(50);
  } while (heading != lastAngle);
  return true;
}

/**
 * Turn a given number of degrees clockwise or counterclockwise.
 *
 * @param angle The number of degrees to turn, positive is counter-clockwise.
 * @return Whether we successfully turned.
 */
bool turnDegrees(int angle)
{
  return turnToHeading(HTMCreadHeading(sensorCompass)-angle);
}

/**
 * Start the sweeper.
 */
void sweeperOn()
{
#ifdef CONNECTION_DETECTION
  if (kInFailureMode)
    return;
#endif
  servo[servoSweeper] = 0;
}

/**
 * Stop the sweeper.
 */
void sweeperOff()
{
  // We're not doing connection detection, since this is perfectly safe to do,
  // even if we lost connection (we're stopping things, not starting things).
  servo[servoSweeper] = 128;
}

/**
 * Reverse the sweeper
 */
void sweeperReverse()
{
#ifdef CONNECTION_DETECTION
  if (kInFailureMode)
    return;
#endif
  servo[servoSweeper] = 255;
}

/**
 * Move the arm one step with a given speed.
 *
 * @param speed The speed at which to move (positive is up, negative down).
 * @param stepSize How far to move (one full rotation of the arm is around 3000 steps).
 */
void armStep(int speed, int stepSize)
{
#ifdef CONNECTION_DETECTION
  if (kInFailureMode)
    return;
#endif

  // Find out the direction to move.
  int direction = speed > 0 ? 1 : -1;

  // Reset the motor encoder
  nMotorEncoder[motorArm] = 0;
  wait10Msec(1);

  motor[motorArm] = speed;

  if (direction == 1) {
    while (nMotorEncoder[motorArm] < abs(stepSize))
      wait1Msec(5);
  } else {
    while (nMotorEncoder[motorArm] > -abs(stepSize))
      wait1Msec(5);
  }
}

/**
 * Move the arm down one "step".
 */
void armStepDown()
{
  armStep(-50, 100);
}

/**
 * Move the arm up one "step".
 */
void armStepUp()
{
  armStep(50, 100);
}

#endif // __4560_COMMON_H__
