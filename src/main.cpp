#include <Arduino.h>
#include <Helpers.h>
#include <LinkedList.h>

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

int GetShift(Vector2Int pos)
{
  return pos.x % 8 + (pos.y % 4) * 8;
}

int count;

void BakeData()
{
  //Serial.println("init set zero");

  for (int x = 0; x < bakeDataX; x++)
  {
    for (int y = 0; y < bakeDataY; y++)
    {
        _bakeData[x][y] = 0;
    }
  }

  if (count < 2){
    Serial.println("bake data");
  
    count++;
  }

  for (int i = 0; i < _cellsCount; i++)
  {
    Vector2Int bakeIndex = GetBakeIndex(_cells[i]->Position);
    uint32_t value = _bakeData[bakeIndex.x][bakeIndex.y];

    value |= (uint32_t)(1 << GetShift(_cells[i]->Position));
    _bakeData[bakeIndex.x][bakeIndex.y] = value;
  }
}

bool HasInBakeDataAt(Vector2Int pos)
{
  Vector2Int bakeIndex = GetBakeIndex(pos);
  uint32_t value = _bakeData[bakeIndex.x][bakeIndex.y];

  return (value & (1 << GetShift(pos))) != 0;
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
    
    _map.SetCellToPos(nullptr, cell->Position);
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

Vector2 simulateDirection;

int Sort(Cell*& a, Cell*& b)
{
  float valA = a->Position.x * simulateDirection.x + a->Position.y * simulateDirection.y;
  float valB = b->Position.x * simulateDirection.x + b->Position.y * simulateDirection.y;

  if (valA > valB)
    return -1;

  if (valA < valB)
    return 1;

  return 0;
}

template<typename T>
void bubbleSort(T arr[], int size, int (*cmp)(T&, T&)) {
    for (int i = 0; i < size - 1; ++i) {
        for (int j = 0; j < size - i - 1; ++j) {
            if (cmp(arr[j], arr[j + 1]) > 0) {
                // Обмін елементів, якщо порівняння вказує на необхідність
                T temp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = temp;
            }
        }
    }
}

// Функція для обміну двох елементів масиву
template<typename T>
void swap(T& a, T& b) {
    T temp = a;
    a = b;
    b = temp;
}

// Функція для розділення масиву та повернення позиції опорного елемента
template<typename T>
int partition(T arr[], int low, int high, int (*cmp)(T&, T&)) {
    T pivot = arr[high]; // Опорний елемент
    int i = low - 1; // Індекс меншого елемента

    for (int j = low; j <= high - 1; j++) {
        if (cmp(arr[j], pivot) < 0) {
            i++;
            swap(arr[i], arr[j]);
        }
    }
    swap(arr[i + 1], arr[high]);
    return i + 1;
}

// Основна функція QuickSort
template<typename T>
void quickSort(T arr[], int low, int high, int (*cmp)(T&, T&)) {
    if (low < high) {
        // Розділити та отримати позицію опорного елемента
        int pi = partition(arr, low, high, cmp);

        // Рекурсивно сортувати дві половини
        quickSort(arr, low, pi - 1, cmp);
        quickSort(arr, pi + 1, high, cmp);
    }
}

template <typename T>
void sortArray(T *array, int size, int (*cmp)(T &, T &)) {
  for (int i = 0; i < size - 1; i++) {
    for (int j = 0; j < size - i - 1; j++) {
      if (cmp(array[j], array[j + 1]) > 0) {
        T temp = array[j];
        array[j] = array[j + 1];
        array[j + 1] = temp;
      }
    }
  }
}

void Simulate(Vector2 direction)
{
  //Serial.println("test - 1");

  BakeData();

  //Serial.println("test - 2");

  simulateDirection = direction;

  //bubbleSort(_cells, _cellsCount, Sort);
  //quickSort(_cells, 0, _cellsCount - 1, Sort);
  //sortArray(_cells, _cellsCount, Sort);

  float angleInDegrees = DirectionToDegrees(direction);

    for (int i = 0; i < _cellsCount; i++)
    {
      Cell* cell = _cells[i];

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
          _map.SetCellToPos(nullptr, cell->Position);
          _map.SetCellToPos(cell, nextPos);

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
  Serial.begin(9600);
  Serial.println("test - 0");

  delay(100);

  Serial.println("test - 1");

  delay(100);

  Serial.println("test - 2");

  delay(100);

  Serial.println("test - 3");

  delay(100);

  // put your setup code here, to run once:
  for (int x = 1; x < 7; x++)
  {
    for (int y = 9; y < 15; y++)
    {
      Vector2Int pos = Vector2Int(x, y);

      Cell* cell = new Cell();
      cell->Position = pos;

      _cells[_cellsCount] = cell;
      _cellsCount++;

      _map.SetCellToPos(cell, pos);
    }
  }

  Serial.println("test - 4");
  delay(100);

  Vector2Int min = Vector2Int(0, 0);
  Vector2Int max = Vector2Int(15, 15);

  bakeDataX = max.x - min.x + 1;
  bakeDataY = max.y - min.y + 1;

  _bakeData = new uint32_t*[bakeDataX];

  for (int i = 0; i < bakeDataX; i++)
  {
    _bakeData[i] = new uint32_t[bakeDataY];
  }

  
  Serial.println("test - 5");
}

float angle = 90;

void loop()
{
  /*
  for (size_t i = 0; i < 10; i++)
  {
    long time = millis();

    //Serial.println("test - 0");

    Simulate(AngleToDirection(angle));

    long dif = millis() - time;
    Serial.println(dif);
  }
  

  angle += 10;
  return;
*/

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

    Simulate(AngleToDirection(angle));

    delay(200);
  }

  angle += 45;
}