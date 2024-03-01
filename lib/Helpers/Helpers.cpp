#pragma once
#include <Helpers.h>

float DirectionToDegrees(Vector2 direction)
{
  // Визначення арктангенса та конвертація радіан в градуси
  float angleInRadians = atan2f(direction.y, direction.x);
  float angleInDegrees = (angleInRadians / PI) * 180;

  // Коригування від'ємних кутів
  if (angleInDegrees < 0)
  {
    angleInDegrees += 360;
  }

  return angleInDegrees;
}

Vector2 AngleToDirection(float degrees)
{
  // Перетворення градусів в радіани
  float angleRadians = degrees * DEG_TO_RAD;

  // Визначення компонент вектора за допомогою тригонометричних функцій
  float x = cosf(angleRadians);
  float y = sinf(angleRadians);

  return Vector2(x, y);
}