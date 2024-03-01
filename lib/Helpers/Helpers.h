// Helpers.h
#pragma once
#include <Arduino.h>

#ifndef HELPERS_H
#define HELPERS_H

struct Vector2
{
  public:

  float x;
  float y;

  Vector2() {};
  Vector2(float _x, float _y) : x(_x), y(_y) {}

  // Перевизначення оператора ==
  bool operator==(const Vector2 &other) const
  {
    return x == other.x && y == other.y;
  }

  // Перевизначення оператора !=
  bool operator!=(const Vector2 &other) const
  {
    return !(*this == other);
  }

  // Додавання
  Vector2 operator+(const Vector2 &other) const
  {
    return Vector2(x + other.x, y + other.y);
  }

  // Віднімання
  Vector2 operator-(const Vector2 &other) const
  {
    return Vector2(x - other.x, y - other.y);
  }

  // Множення на скаляр
  Vector2 operator*(float scalar) const
  {
    return Vector2(x * scalar, y * scalar);
  }

  // Ділення на скаляр
  Vector2 operator/(float scalar) const
  {
    // Додайте перевірку на поділ на нуль, якщо потрібно
    return Vector2(x / scalar, y / scalar);
  }

  float length() const
  {
    return sqrtf(x * x + y * y);
  }

  // Метод для нормалізації вектора
  void normalize()
  {
    float len = length();
    if (len != 0)
    {
      x /= len;
      y /= len;
    }
  }

  Vector2 normalized()
  {
    float len = length();
    if (len != 0)
    {
      return Vector2(x / len, y / len);
    }
  }

  float distance(const Vector2 &other) const
  {
    int dx = x - other.x;
    int dy = y - other.y;
    return sqrt(dx * dx + dy * dy);
  }
};

// Структура для представлення двовимірного вектора з цілими числами
struct Vector2Int
{
  public:

  int x;
  int y;

  Vector2Int() {};
  Vector2Int(int _x, int _y) : x(_x), y(_y) {}

  // Перевизначення оператора ==
  bool operator==(const Vector2Int &other) const
  {
    return x == other.x && y == other.y;
  }

  // Перевизначення оператора !=
  bool operator!=(const Vector2Int &other) const
  {
    return !(*this == other);
  }

  // Додавання
  Vector2Int operator+(const Vector2Int &other) const
  {
    return Vector2Int(x + other.x, y + other.y);
  }

  // Віднімання
  Vector2Int operator-(const Vector2Int &other) const
  {
    return Vector2Int(x - other.x, y - other.y);
  }

  // Множення на скаляр
  Vector2Int operator*(int scalar) const
  {
    return Vector2Int(x * scalar, y * scalar);
  }

  // Ділення на скаляр
  Vector2Int operator/(int scalar) const
  {
    // Додайте перевірку на поділ на нуль, якщо потрібно
    return Vector2Int(x / scalar, y / scalar);
  }
};

struct Cell
{
  bool IsFalling;
  int8_t Index;
  int8_t FallIndex;

  Vector2Int StartFallPosition = Vector2Int();
  Vector2Int EndFallPosition = Vector2Int();
    
  Vector2Int Position = Vector2Int();
};

static Vector2Int _directions[] =
{
  Vector2Int(1, 0),
  Vector2Int(1, 1),
  Vector2Int(0, 1),
  Vector2Int(-1, 1),
  Vector2Int(-1, 0),
  Vector2Int(-1, -1),
  Vector2Int(0, -1),
  Vector2Int(1, -1),
};

float DirectionToDegrees(Vector2 direction);
Vector2 AngleToDirection(float degrees);

#endif // HELPERS_H