#pragma once

#include <Helpers.h>

void InitSand();
void ResetSand();
void Simulate(Vector2 direction);
bool IsPositionInSand(Vector2Int pos);
Cell* GetSandCellAtPos(Vector2Int pos);

void BakeData();
uint32_t** GetBakeData();

int GetPartsAmount();

void SetTransferEnabled(bool enabled);
bool IsTransferEnabled();