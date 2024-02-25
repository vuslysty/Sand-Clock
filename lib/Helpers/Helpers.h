// Helpers.h

#ifndef HELPERS_H
#define HELPERS_H

// Ваші визначення нових типів даних тут
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

  Vector2Int StartFallPosition = Vector2Int();
  Vector2Int EndFallPosition = Vector2Int();
    
  Vector2Int Position = Vector2Int();
};

class LinePointsIterator {
public:
    LinePointsIterator(const Vector2Int& start, const Vector2Int& end)
        : current(start), end(end) {}

    bool operator!=(const LinePointsIterator& other) const {
        return current.x != other.end.x || current.y != other.end.y;
    }

    const Vector2Int& operator*() const {
        return current;
    }

    LinePointsIterator& operator++() {
        int x0 = current.x;
        int y0 = current.y;
        int x1 = end.x;
        int y1 = end.y;
        int sx = x0 < x1 ? 1 : -1;
        int sy = y0 < y1 ? 1 : -1;
        int dx = abs(x1 - x0);
        int dy = abs(y1 - y0);

        if (dy <= dx) {
            int d = (dy << 1) - dx;
            int d1 = dy << 1;
            int d2 = (dy - dx) << 1;
            if (current.x != end.x) {
                current.x += sx;
                if (d > 0) {
                    d += d2;
                    current.y += sy;
                } else
                    d += d1;
            }
        } else {
            int d = (dx << 1) - dy;
            int d1 = dx << 1;
            int d2 = (dx - dy) << 1;
            if (current.y != end.y) {
                current.y += sy;
                if (d > 0) {
                    d += d2;
                    current.x += sx;
                } else
                    d += d1;
            }
        }
        return *this;
    }

private:
    Vector2Int current;
    Vector2Int end;
};

class LinePoints {
public:
    LinePoints(const Vector2Int& start, const Vector2Int& end)
        : start_point(start), end_point(end) {}

    LinePointsIterator begin() const {
        return LinePointsIterator(start_point, end_point);
    }

    LinePointsIterator end() const {
        return LinePointsIterator(end_point, end_point);
    }

private:
    Vector2Int start_point;
    Vector2Int end_point;
};

#endif // HELPERS_H