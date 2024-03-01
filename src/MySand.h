#pragma once

#include <Helpers.h>

void InitSand();
void Simulate(Vector2 direction);
bool IsPositionInSand(Vector2Int pos);
Cell* GetSandCellAtPos(Vector2Int pos);