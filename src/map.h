#pragma once

#include <Arduino.h>
#include <Helpers.h>

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

typedef Cell* (*GetCellByIndexCb)(int index);

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

    return index >= 0 && getCellCallback != nullptr ? getCellCallback(index) : nullptr;
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

  void AttachGetCell(GetCellByIndexCb cb)
  {
    getCellCallback = cb;
  }

  private:
  GetCellByIndexCb getCellCallback = nullptr;
};