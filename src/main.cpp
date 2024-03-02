#include <Arduino.h>
#include "Map.h"
#include "MySand.h"
#include "print3x5.h"

#define BTN1_PIN 2
#define BTN2_PIN 3
#define CS_PIN 6
#define DT_PIN 4
#define CK_PIN 5

// memory
struct Data {
    int16_t sec = 60;
    int8_t bri = 5;
};
Data data;

#include <EEManager.h>
EEManager memory(data);

// button
#include <EncButton.h>
Button up(BTN1_PIN);
Button down(BTN2_PIN);
VirtButton dbl;

// matrix
#include <GyverMAX7219.h>
MAX7219<2, 1, CS_PIN, DT_PIN, CK_PIN> mtrx;

// mpu
#include "mini6050.h"
Mini6050 mpu;

// timer
#include "Timer.h"
Timer fall_tmr, disp_tmr;

void setValueToPosInMatrix(int X, int Y, uint32_t value)
{
  /*
  Serial.print("In: ");
    Serial.print("X: ");
    Serial.print(X);
    Serial.print(" Y: ");
    Serial.print(Y);
    Serial.print(" Val: ");
    Serial.println(value);
  */

  for (int i = 0; i < 32; i++)
  {
    mtrx.dot(X + i % 8, Y + 3 - (i / 8), bitRead(value, i));

  /*
    Serial.print("Out: ");
    Serial.print("X: ");
    Serial.print(X + i % 8);
    Serial.print(" Y: ");
    Serial.print(Y + 3 - (i / 8));
    Serial.print(" Val: ");
    Serial.println(bitRead(value, i));
    */
  }
}

void updateSand()
{
  mtrx.clear();

  setValueToPosInMatrix(0, 0, GetBakeData()[0][3]);
  setValueToPosInMatrix(0, 4, GetBakeData()[0][2]);
  setValueToPosInMatrix(8, 0, GetBakeData()[1][1]);
  setValueToPosInMatrix(8, 4, GetBakeData()[1][0]);
  
  mtrx.update();
}

void resetSand()
{
    ResetSand();
    updateSand();
}

void changeTime(int8_t dir)
{
    disp_tmr.setTimeout(3000);
    mtrx.clear();
    data.sec += dir;
    if (data.sec < 0) data.sec = 0;
    uint8_t min = data.sec / 60;
    uint8_t sec = data.sec % 60;

    printDig(&mtrx, 0, 1, min / 10);
    printDig(&mtrx, 4, 1, min % 10);
    printDig(&mtrx, 8 + 0, 1, sec / 10);
    printDig(&mtrx, 8 + 4, 1, sec % 10);

    fall_tmr.setInterval(data.sec * 1000ul / GetPartsAmount());
    memory.update();
    mtrx.update();
}

void changeBri(int8_t dir) 
{
    data.bri += dir;
    data.bri = constrain(data.bri, 0, 15);
    mtrx.setBright(data.bri);
    memory.update();
}

void buttons() 
{
    up.tick();
    down.tick();
    dbl.tick(up, down);

    if (dbl.click()) resetSand();

    if (up.click()) changeTime(1);
    if (up.step(0)) changeTime(10);
    if (up.step(1)) changeBri(1);

    if (down.click()) changeTime(-1);
    if (down.step(0)) changeTime(-10);
    if (down.step(1)) changeBri(-1);
}

void step()
{
    uint16_t prd = 255 - mpu.getMag();
    prd = constrain(prd, 15, 90);

    if (mpu.update(prd) || IsTransferEnabled())
    {
       bool upBeforeState = GetBakeData()[0][2] || GetBakeData()[0][3];
       bool downBeforeState = GetBakeData()[1][0] || GetBakeData()[1][1];

        Vector2 direction = AngleToDirection(mpu.getAngle() - 45);
        Simulate(direction);

        bool upAfterState = GetBakeData()[0][2] || GetBakeData()[0][3];
        bool downAfterState = GetBakeData()[1][0] || GetBakeData()[1][1];

        if ((upBeforeState && !upAfterState) || (downBeforeState && !downAfterState))
          Serial.println("end");

        updateSand();
    }
}

void setup()
{
  Serial.begin(115200);

  InitSand();

  Wire.begin();
  mpu.begin();
  memory.begin(0, 'a');
  mtrx.begin();
  mtrx.setBright(data.bri);

  mpu.setX({1, -1});
  mpu.setY({2, 1});
  mpu.setZ({0, 1});

  fall_tmr.setInterval(data.sec * 1000ul / GetPartsAmount());
}

float angle = 90;

void loop()
{
  memory.tick();
  disp_tmr.tick();
  buttons();

  if (!disp_tmr.state())
  {
    if (fall_tmr)
      SetTransferEnabled(true);
    
    step();
    SetTransferEnabled(false);
  }

  return;


  for (size_t i = 0; i < 10; i++)
  {
    long time = millis();

    Simulate(AngleToDirection(angle));

    long dif = millis() - time;
    Serial.println(dif);
  }
  

  angle += 10;
  
  if (angle >= 360)
    angle -= 360;

  //Serial.println(angle);

  return;

  for (size_t i = 0; i < 10; i++)
  {
    for (int y = 16; y >= -1; y--)
    {
      for (int x = -1; x <= 16; x++)
      {
        Vector2Int pos = Vector2Int(x, y);

        if (!IsPositionInSand(pos))
        {
          Serial.print('#');
          continue;
        }

        Serial.print(GetSandCellAtPos(pos) == nullptr ? ' ' : 'X');
      }

      Serial.println();
    }

    Serial.println();

    Simulate(AngleToDirection(angle));
    updateSand();

    delay(200);
  }

  angle += 45;

//JnU9MzkxMTkxNzY4ODY3MzUzNjAxJm49JUQwJTkyJUQwJUJCJUQwJUIwJUQwJUI0JUQwJUI4JUQxJTgxJUQwJUJCJUQwJUIwJUQwJUIyKyVEMCVBMyVEMSU4MSVEMCVCQiVEMCVCOCVEMSU4MSVEMSU4MiVEMCVCOCVEMCVCOSZlPXZsYWR1dmkyMyU0MGdtYWlsLmNvbSZ4PTIwMjQwMzMxAIjFftK8I1nNDZ54xm21N3i4IRw3d5TGippDCHUcvqAeQWSgLKJkU85c2Vyw9Tp4WQQDflqKW5zeW11bVFPYFkfzimOnvYvx7PUQQ8SNN_PIJixQHpa86TH14s_SlKbcCTPx3EjG6dA6BV4DNL23pc5YFJxJOGXwaMI5mSUe_SUdl079AUoYPn0DjItxrSfdSrNy2SrmC_P7spD86uxpaV0gwkSG_P4_SUbEIR_PL8iXzMI_SOGrh2kR86qxhAcpfkLLoZv9BLFYJbNRGxB1q8vPB7Lld8_SUXOX62MysNxeDcgd6TpyDyvwoMB9P_Py9TS26wzmp9aqdHQ9R5QRfETczq_SotgSzI
}