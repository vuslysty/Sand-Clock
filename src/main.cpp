#include <Arduino.h>
#include <Helpers.h>

// put function declarations here:
int myFunction(int, int);

const Vector2Int _directions[] =
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

Vector2Int* _buffer = (Vector2Int*)malloc(4 * sizeof(Vector2Int));
Cell* _cells [64];
int _cellsCount = 0;

uint32_t** _bakeData = nullptr;
int bakeDataX;
int bakeDataY;

Vector2Int GetBakeIndex(Vector2Int pos)
{
  return Vector2Int(pos.x / 8, pos.y / 4);
}

uint32_t GetShift(Vector2Int pos)
{
  return pos.x % 8 + (pos.y % 4) * 8;
}

void BakeData()
{
  for (int x = 0; x < bakeDataX; x++)
  {
    for (int y = 0; y < bakeDataY; y++)
    {
        _bakeData[x][y] = 0;
    }
  }

  for (int i = 0; i < _cellsCount; i++)
  {
    Vector2Int bakeIndex = GetBakeIndex(_cells[i]->Position);

    if (_cells[i]->Position.x < 0 || _cells[i]->Position.x >= bakeDataX)
      Serial.println("fail-1");

    if (_cells[i]->Position.y < 0 || _cells[i]->Position.y >= bakeDataY)
      Serial.println("fail-2");

    uint32_t value = _bakeData[bakeIndex.x][bakeIndex.y];

    value |= 1u << GetShift(_cells[i]->Position);
    _bakeData[bakeIndex.x][bakeIndex.y] = value;
  }
}

bool HasInBakeDataAt(Vector2Int pos)
{
  Vector2Int bakeIndex = GetBakeIndex(pos);
  uint32_t value = _bakeData[bakeIndex.x][bakeIndex.y];

  return (value & (1u << GetShift(pos))) != 0;
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

class RectMap
{
  int8_t **_array;
  Vector2Int _pos;
  Vector2Int _size;

  public:

  RectMap()
  {

  }

  RectMap(Vector2Int pos, Vector2Int size)
  {
    _pos = pos;
    _size = size;

    _array = new int8_t*[size.x];

    for (int x = 0; x < size.x; x++)
    {
      _array[x] = new int8_t[size.y];

      for (int y = 0; y < size.y; y++)
      {
        _array[x][y] = -1;
      }
    }
  }

  bool Contains(Vector2Int pos)
  {
    return  pos.x >= _pos.x && pos.x <= _pos.x + _size.x - 1 &&
            pos.y >= _pos.y && pos.y <= _pos.y + _size.y - 1;
  }

  int8_t GetCellIndex(Vector2Int pos)
  {
    if (!Contains(pos))
      return -1;

    Vector2Int localPos = GetLocalPosition(pos);

    return _array[localPos.x][localPos.y];
  }

  void SetCellIndexToPos(int8_t cellIndex, Vector2Int pos)
  {
    Vector2Int localPos = GetLocalPosition(pos);
    _array[localPos.x][localPos.y] = cellIndex;
  }

  void ClearPos(Vector2Int pos)
  {
    SetCellIndexToPos(-1, pos);
  }

  private:

  Vector2Int GetLocalPosition(Vector2Int globalPos)
  {
    return Vector2Int(globalPos.x - _pos.x, globalPos.y - _pos.y);
  }
};

class Map
{
  RectMap _rects[2];
  int rectsSize;

public:
  Map()
  {
    _rects[0] = RectMap(Vector2Int(0, 8), Vector2Int(8, 8));
    _rects[1] = RectMap(Vector2Int(8, 0), Vector2Int(8, 8));
  
    rectsSize = 2;
  }

  bool Contains(Vector2Int pos)
  {
    for (int i = 0; i < rectsSize; i++)
    {
      if (_rects[i].Contains(pos))
        return true;
    }

    return false;
  }

  Cell* GetCell(Vector2Int pos)
  {
    int8_t index = -1;

    for (int i = 0; i < rectsSize; i++)
    {
      if (!_rects[i].Contains(pos))
        continue;

      index = _rects[i].GetCellIndex(pos);
      break;
    }

    return index < 0 ? nullptr : _cells[index];
  }

  void SetCellToPos(Cell* cell, Vector2Int pos)
  {
    for (int i = 0; i < rectsSize; i++)
    {
      if (!_rects[i].Contains(pos))
        continue;

      _rects[i].SetCellIndexToPos(cell == nullptr ? -1 : cell->Index, pos);
    }
  }

  void ClearPos(Vector2Int pos)
  {
    for (int i = 0; i < rectsSize; i++)
    {
      if (!_rects[i].Contains(pos))
        continue;

      _rects[i].ClearPos(pos);
    }
  }
};

Map _map = Map();

Vector2Int GetEndFallPos(Vector2Int startFallPos, Vector2 direction)
{
  Vector2 startFallPosCenter = Vector2(startFallPos.x, startFallPos.y);
  startFallPosCenter = startFallPosCenter + Vector2(0.5f, 0.5f);

  Vector2 endFallCenter = startFallPosCenter + direction * 10;
  Vector2Int endFallPos = Vector2Int((int)endFallCenter.x, (int)endFallCenter.y);

  return endFallPos;
}

const int GetIndexForAngle(float angleInDegrees)
{
  if (angleInDegrees >= 0 && angleInDegrees < 90)
    return 1;
  if (angleInDegrees >= 90 && angleInDegrees < 180)
    return 3;
  if (angleInDegrees >= 180 && angleInDegrees < 270)
    return 5;
  if (angleInDegrees >= 270 && angleInDegrees < 360)
    return 7;

  return 1;
}

int GetIndexOffset(int index, int offset)
{
  return ((index + offset) % 8 + 8) % 8;
}

int GetSortedMovePoints(float angleInDegrees)
{
  const float N = 1;

  const int quadrantIndex = GetIndexForAngle(angleInDegrees);
  int angleFrom0To90 = ((int)angleInDegrees) % 90;

  float distToFirst = abs(angleInDegrees - 0);
  float distToSecond = abs(angleInDegrees - 45);
  float distToThird = abs(angleInDegrees - 90);

  int amount;

  if (angleFrom0To90 >= 0 && angleFrom0To90 < N)
  {
    if (distToFirst < distToSecond)
    {
      _buffer[0] = _directions[GetIndexOffset(quadrantIndex, -1)];
      _buffer[1] = _directions[quadrantIndex];
    }
    else
    {
      _buffer[0] = _directions[quadrantIndex];
      _buffer[1] = _directions[GetIndexOffset(quadrantIndex, -1)];
    }

    amount = 2;
  }
  else if (angleFrom0To90 >= 90 - N && angleFrom0To90 < 90)
  {
    if (distToSecond < distToThird)
    {
      _buffer[0] = _directions[quadrantIndex];
      _buffer[1] = _directions[GetIndexOffset(quadrantIndex, 1)];
    }
    else
    {
      _buffer[0] = _directions[GetIndexOffset(quadrantIndex, 1)];
      _buffer[1] = _directions[quadrantIndex];
    }

    amount = 2;
  }
  else
  {
    if (angleFrom0To90 > 45)
    {
      if (distToSecond < distToThird)
      {
        _buffer[0] = _directions[quadrantIndex];
        _buffer[1] = _directions[GetIndexOffset(quadrantIndex, 1)];
      }
      else
      {
        _buffer[0] = _directions[GetIndexOffset(quadrantIndex, 1)];
        _buffer[1] = _directions[quadrantIndex];
      }

      _buffer[2] = _directions[GetIndexOffset(quadrantIndex, -1)];
    }
    else
    {
      if (distToFirst < distToSecond)
      {
        _buffer[0] = _directions[GetIndexOffset(quadrantIndex, -1)];
        _buffer[1] = _directions[quadrantIndex];
      }
      else
      {
        _buffer[0] = _directions[quadrantIndex];
        _buffer[1] = _directions[GetIndexOffset(quadrantIndex, -1)];
      }

      _buffer[2] = _directions[GetIndexOffset(quadrantIndex, 1)];
    }

    amount = 3;
  }

  if (angleFrom0To90 > 50)
  {
    _buffer[amount] = _directions[GetIndexOffset(quadrantIndex, 2)];
    amount++;
  }
  else if (angleFrom0To90 < 40)
  {
    _buffer[amount] = _directions[GetIndexOffset(quadrantIndex, -2)];
    amount++;
  }

  return amount;
}

bool TryMakeMove(Cell* cell, float angleInDegrees, Vector2Int exceptPoint)
{
  int points = GetSortedMovePoints(angleInDegrees);

  for (int i = 0; i < points; i++)
  {
    Vector2Int checkPoint = cell->Position + _buffer[i];

    if (checkPoint == exceptPoint)
      continue;

    if (!_map.Contains(checkPoint))
      continue;

    Cell* nextCell = _map.GetCell(checkPoint);

    if (nextCell != nullptr)
      continue;

    if (HasInBakeDataAt(checkPoint))
      return false;
    
    _map.ClearPos(cell->Position);
    _map.SetCellToPos(cell, checkPoint);
    cell->Position = checkPoint;
    
    return true;
  }

  return true;
}

static float DirectionToDegrees(Vector2 direction)
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

static Vector2Int GetLinePointAtIteration(Vector2Int start, Vector2Int end, int targetIteration)
{
  if (targetIteration <= 0)
    return start;

  int iterations = 0;

  int x0 = start.x;
  int y0 = start.y;
  int x1 = end.x;
  int y1 = end.y;
  int sx = x0 < x1 ? 1 : -1;
  int sy = y0 < y1 ? 1 : -1;
  int dx = abs(x1 - x0);
  int dy = abs(y1 - y0);

  if (dy <= dx)
  {
    int d = (dy << 1) - dx;
    int d1 = dy << 1;
    int d2 = (dy - dx) << 1;

    for (int x = x0 + sx, y = y0, i = 1; i <= dx; i++, x += sx)
    {
      if (d > 0)
      {
        d += d2;
        y += sy;
      }
      else
        d += d1;

      iterations++;

      if (iterations == targetIteration)
        return Vector2Int(x, y);
    }
  }
  else
  {
    int d = (dx << 1) - dy;
    int d1 = dx << 1;
    int d2 = (dx - dy) << 1;

    for (int y = y0 + sy, x = x0, i = 1; i <= dy; i++, y += sy)
    {
      if (d > 0)
      {
        d += d2;
        x += sx;
      }
      else
        d += d1;

      iterations++;

      if (iterations == targetIteration)
        return Vector2Int(x, y);
    }
  }

  return end;
}

static Vector2Int GetLinePointAtIteration(Vector2Int start, Vector2 direction, int targetIteration)
{
  int x0 = start.x;
  int y0 = start.y;

  int x1 = (int)(x0 + targetIteration * 10 * direction.x);
  int y1 = (int)(y0 + targetIteration * 10 * direction.y);

  return GetLinePointAtIteration(start, Vector2Int(x1, y1), targetIteration);
}

void Simulate(Vector2 direction)
{
  BakeData();

  float angleInDegrees = DirectionToDegrees(direction);

  for (int i = 0; i < _cellsCount; i++)
  {
    Cell *cell = _cells[i];

    // Try stop falling
    if (cell->IsFalling)
    {
      Vector2Int endFallPos = GetEndFallPos(cell->StartFallPosition, direction);

      if (endFallPos != cell->EndFallPosition)
        cell->IsFalling = false;
    }

    Vector2Int nextPos = cell->IsFalling
                ? GetLinePointAtIteration(cell->StartFallPosition, direction, cell->FallIndex + 1)
                : GetLinePointAtIteration(cell->Position, direction, 1);

    if (_map.Contains(nextPos))
    {
      Cell *nextPosCell = _map.GetCell(nextPos);

      if (nextPosCell != nullptr)
      {
        if (TryMakeMove(cell, angleInDegrees, nextPos))
        {
          cell->IsFalling = false;
        }
      }
      else if (HasInBakeDataAt(nextPos))
      {
        continue;
      }
      else
      {
        _map.ClearPos(cell->Position);
        _map.SetCellToPos(cell, nextPos);

        if (!cell->IsFalling)
        {
          cell->IsFalling = true;
          cell->FallIndex = 1;
          cell->StartFallPosition = cell->Position;
          cell->EndFallPosition = GetEndFallPos(cell->Position, direction);
        }
        else
        {
          cell->FallIndex++;
        }

        cell->Position = nextPos;
      }
    }
    else
    {
      // Need add a logic for falling in right place
      if (TryMakeMove(cell, angleInDegrees, nextPos))
      {
        cell->IsFalling = false;
      }
    }
  }
}

void setup()
{
  Serial.begin(115200);

  // put your setup code here, to run once:
  for (int x = 1; x < 7; x++)
  {
    for (int y = 9; y < 15; y++)
    {
      Vector2Int pos = Vector2Int(x, y);

      Cell* cell = new Cell();
      cell->Position = pos;
      cell->Index = _cellsCount;

      _cells[_cellsCount] = cell;
      _cellsCount++;

      _map.SetCellToPos(cell, pos);
    }
  }

  Vector2Int min = Vector2Int(0, 0);
  Vector2Int max = Vector2Int(15, 15);

  bakeDataX = max.x - min.x + 1;
  bakeDataY = max.y - min.y + 1;

  _bakeData = new uint32_t*[bakeDataX];

  for (int i = 0; i < bakeDataX; i++)
  {
    _bakeData[i] = new uint32_t[bakeDataY];
  }
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

        if (!_map.Contains(pos))
        {
          Serial.print('#');
          continue;
        }

        Serial.print(_map.GetCell(pos) == nullptr ? ' ' : 'X');
      }

      Serial.println();
    }

    Serial.println();

    Simulate(AngleToDirection(angle));

    delay(200);
  }

  angle += 45;
}