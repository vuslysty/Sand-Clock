#include <Arduino.h>
#include "Timer.h"
#include "Map.h"
#include "MySand.h"

void setup()
{
  Serial.begin(115200);

  InitSand();
}

float angle = 90;

void loop()
{
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

    delay(200);
  }

  angle += 45;
}