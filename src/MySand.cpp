#include "MySand.h"
#include "Map.h"
#include <Helpers.h>
#include <Arduino.h>

Vector2Int* _buffer = (Vector2Int*)malloc(4 * sizeof(Vector2Int));
Cell* _cells [64];
int _cellsCount = 0;

uint32_t** _bakeData = nullptr;
int bakeDataX;
int bakeDataY;

Map _map = Map();

Cell* GetCellByIndex(int index)
{
  return _cells[index];
}

void ResetSand()
{
    
}

void InitSand()
{
  _map.AttachGetCell(GetCellByIndex);

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

bool IsPositionInSand(Vector2Int pos)
{
  return _map.Contains(pos);
}

Cell* GetSandCellAtPos(Vector2Int pos)
{
  return _map.GetCell(pos);
}

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