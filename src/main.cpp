#include <Arduino.h>
#include <Helpers.h>
#include <LinkedList.h>

// put function declarations here:
int myFunction(int, int);

const Vector2Int quadrant1[] = { Vector2Int(1, 0), Vector2Int(1, 1), Vector2Int(0, 1), Vector2Int(-1, 1), Vector2Int(1, -1) };
const Vector2Int quadrant2[] = { Vector2Int(0, 1), Vector2Int(-1, 1), Vector2Int(-1, 0), Vector2Int(-1, -1), Vector2Int(1, 1) };
const Vector2Int quadrant3[] = { Vector2Int(-1, 0), Vector2Int(-1, -1), Vector2Int(0, -1), Vector2Int(1, -1), Vector2Int(-1, 1) };
const Vector2Int quadrant4[] = { Vector2Int(0, -1), Vector2Int(1, -1), Vector2Int(1, 0), Vector2Int(1, 1), Vector2Int(-1, -1) };

Vector2Int* _buffer = (Vector2Int*)malloc(4 * sizeof(Vector2Int));

LinkedList<Cell*> _cells = LinkedList<Cell*>();

class RectMap
{
  Cell ***_array;
  Vector2Int _pos;
  Vector2Int _size;

  public:

  RectMap(Vector2Int pos, Vector2Int size)
  {
    _pos = pos;
    _size = size;

    _array = new Cell**[size.x];

    for (int x = 0; x < size.x; x++)
    {
      _array[x] = new Cell*[size.y];

      for (int y = 0; y < size.y; y++)
      {
        _array[x][y] = nullptr;
      }
    }
  }

  bool Contains(Vector2Int pos)
  {
    return  pos.x >= _pos.x && pos.x <= _pos.x + _size.x - 1 &&
            pos.y >= _pos.y && pos.y <= _pos.y + _size.y - 1;
  }

  Cell* GetCell(Vector2Int pos)
  {
    if (!Contains(pos))
      return nullptr;

    Vector2Int localPos = GetLocalPosition(pos);

    return _array[localPos.x][localPos.y];
  }

  void SetCellToPos(Cell* cell, Vector2Int pos)
  {
    Vector2Int localPos = GetLocalPosition(pos);
    _array[localPos.x][localPos.y] = cell;
  }

  void ClearPos(Vector2Int pos)
  {
    SetCellToPos(nullptr, pos);
  }

  private:

  Vector2Int GetLocalPosition(Vector2Int globalPos)
  {
    return Vector2Int(globalPos.x - _pos.x, globalPos.y - _pos.y);
  }
};

class Map
{
  LinkedList<RectMap*> _rects = LinkedList<RectMap*>();

public:
  Map()
  {
    _rects.add(new RectMap(Vector2Int(0, 8), Vector2Int(8, 8)));
    _rects.add(new RectMap(Vector2Int(8, 0), Vector2Int(8, 8)));
  }

  bool Contains(Vector2Int pos)
  {
    for (int i = 0; i < _rects.size(); i++)
    {
      if (_rects[i]->Contains(pos))
        return true;
    }

    return false;
  }

  Cell* GetCell(Vector2Int pos)
  {
    Cell* cell = nullptr;

    for (int i = 0; i < _rects.size(); i++)
    {
      cell = _rects[i]->GetCell(pos);

      if (cell != nullptr)
        return cell;
    }

    return nullptr;
  }

  void SetCellToPos(Cell* cell, Vector2Int pos)
  {
    for (int i = 0; i < _rects.size(); i++)
    {
      if (!_rects[i]->Contains(pos))
        continue;

      _rects[i]->SetCellToPos(cell, pos);
    }
  }

  void ClearPos(Vector2Int pos)
  {
    for (int i = 0; i < _rects.size(); i++)
    {
      if (!_rects[i]->Contains(pos))
        continue;

      _rects[i]->ClearPos(pos);
    }
  }
};

Map _map = Map();

Vector2Int GetEndFallPos(Vector2Int startFallPos, Vector2 direction)
{
  Vector2 startFallPosCenter = Vector2(startFallPos.x, startFallPos.y);
  startFallPosCenter = startFallPosCenter + Vector2(0.5f, 0.5f);

  Vector2 endFallCenter = startFallPosCenter + direction * 100;
  Vector2Int endFallPos = Vector2Int((int)endFallCenter.x, (int)endFallCenter.y);

  return endFallPos;
}

Vector2Int GetNextPoint(Vector2Int currentPoint, Vector2 direction)
{
  direction.normalize();

  int x0 = currentPoint.x;
  int y0 = currentPoint.y;

  int count = 1000;

  int x1 = (int)(x0 + (count - 1) * direction.x);
  int y1 = (int)(y0 + (count - 1) * direction.y);

  LinePoints line(currentPoint, Vector2Int(x1, y1));

  for (const auto& point : line)
  {
      if (point == currentPoint)
      continue;

    return point;
  }

  return Vector2Int();
}

Vector2Int GetNextPointWhenFalling(Vector2Int currentPoint, Vector2Int startFallPoint, Vector2 direction)
{
  direction.normalize();

  int x0 = startFallPoint.x;
  int y0 = startFallPoint.y;

  int count = 1000;

  int x1 = (int)(x0 + (count - 1) * direction.x);
  int y1 = (int)(y0 + (count - 1) * direction.y);

  float prevDistance = __FLT_MAX__;

  LinePoints line(startFallPoint, Vector2Int(x1, y1));

  for (const auto& point : line)
  {
    float distance = Vector2(point.x, point.y).distance(Vector2(currentPoint.x, currentPoint.y));

    if (distance > prevDistance && distance > 0.5f)
      return point;

    prevDistance = distance;
  }

  return Vector2Int();
}

const Vector2Int* GetQuadrantForAngle(float angleInDegrees)
{
  if (angleInDegrees >= 0 && angleInDegrees < 90)
    return quadrant1;
  if (angleInDegrees >= 90 && angleInDegrees < 180)
    return quadrant2;
  if (angleInDegrees >= 180 && angleInDegrees < 270)
    return quadrant3;
  if (angleInDegrees >= 270 && angleInDegrees < 360)
    return quadrant4;

  return nullptr;
}

int GetSortedMovePoints(float angleInDegrees)
{
  const float N = 1;

  const Vector2Int* quadrant = GetQuadrantForAngle(angleInDegrees);
  int angleFrom0To90 = ((int)angleInDegrees) % 90;

  float distToFirst = abs(angleInDegrees - 0);
  float distToSecond = abs(angleInDegrees - 45);
  float distToThird = abs(angleInDegrees - 90);

  int amount;

  if (angleFrom0To90 >= 0 && angleFrom0To90 < N)
  {
    if (distToFirst < distToSecond)
    {
      _buffer[0] = quadrant[0];
      _buffer[1] = quadrant[1];
    }
    else
    {
      _buffer[0] = quadrant[1];
      _buffer[1] = quadrant[0];
    }

    amount = 2;
  }
  else if (angleFrom0To90 >= 90 - N && angleFrom0To90 < 90)
  {
    if (distToSecond < distToThird)
    {
      _buffer[0] = quadrant[1];
      _buffer[1] = quadrant[2];
    }
    else
    {
      _buffer[0] = quadrant[2];
      _buffer[1] = quadrant[1];
    }

    amount = 2;
  }
  else
  {
    if (angleFrom0To90 > 45)
    {
      if (distToSecond < distToThird)
      {
        _buffer[0] = quadrant[1];
        _buffer[1] = quadrant[2];
      }
      else
      {
        _buffer[0] = quadrant[2];
        _buffer[1] = quadrant[1];
      }

      _buffer[2] = quadrant[0];
    }
    else
    {
      if (distToFirst < distToSecond)
      {
        _buffer[0] = quadrant[0];
        _buffer[1] = quadrant[1];
      }
      else
      {
        _buffer[0] = quadrant[1];
        _buffer[1] = quadrant[0];
      }

      _buffer[2] = quadrant[2];
    }

    amount = 3;
  }

  if (angleFrom0To90 > 50)
  {
    _buffer[amount] = quadrant[3];
    amount++;
  }
  else if (angleFrom0To90 < 40)
  {
    _buffer[amount] = quadrant[4];
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
    {
      if (!nextCell->MakeMove)
        return false;
    }
    else
    {
      _map.SetCellToPos(nullptr, cell->Position);
      _map.SetCellToPos(cell, checkPoint);
      cell->Position = checkPoint;
      cell->MakeMove = true;
      return true;
    }
  }

  cell->MakeMove = true;
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

void Simulate(Vector2 direction)
{
  for (int i = 0; i < _cells.size(); i++)
  {
    _cells[i]->MakeMove = false;
  }
  

  float angleInDegrees = DirectionToDegrees(direction);
  int moves = 0;

  do
  {
    moves = 0;

    for (int i = 0; i < _cells.size(); i++)
    {
      Cell* cell = _cells[i];

      if (cell->MakeMove)
        continue;

      // Try stop falling
      if (cell->IsFalling)
      {
        Vector2Int endFallPos = GetEndFallPos(cell->StartFallPosition, direction);

        if (endFallPos != cell->EndFallPosition)
          cell->IsFalling = false;
      }

      Vector2Int nextPos = cell->IsFalling
        ? GetNextPointWhenFalling(cell->Position, cell->StartFallPosition, direction)
        : GetNextPoint(cell->Position, direction);

      if (_map.Contains(nextPos))
      {
        Cell* nextPosCell = _map.GetCell(nextPos);

        if (nextPosCell != nullptr)
        {
          if (nextPosCell->MakeMove)
          {
            cell->IsFalling = false;

            // Need add a logic for falling in right place
            if (TryMakeMove(cell, angleInDegrees, nextPos))
            {
              moves++;
            }
          }
        }
        else
        {
          _map.SetCellToPos(nullptr, cell->Position);
          _map.SetCellToPos(cell, nextPos);
          moves++;

          cell->MakeMove = true;

          if (!cell->IsFalling)
          {
            cell->IsFalling = true;
            cell->StartFallPosition = cell->Position;
            cell->EndFallPosition = GetEndFallPos(cell->Position, direction);
          }

          cell->Position = nextPos;
        }
      }
      else
      {
        cell->IsFalling = false;

        // Need add a logic for falling in right place
        if (TryMakeMove(cell, angleInDegrees, nextPos))
        {
          moves++;
        }
      }
    }
  } while (moves > 0);
}

void setup()
{
  // put your setup code here, to run once:
  for (int x = 0; x < 8; x++)
  {
    for (int y = 10; y < 14; y++)
    {
      Vector2Int pos = Vector2Int(x, y);

      Cell* cell = new Cell();
      cell->Position = pos;

      _cells.add(cell);
      _map.SetCellToPos(cell, pos);
    }
  }
}

void loop()
{
  
}

// put function definitions here:
int myFunction(int x, int y)
{
  return x + y;
}