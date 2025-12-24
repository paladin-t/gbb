/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#ifndef __SHAPES_H__
#define __SHAPES_H__

#include "../gbbasic.h"
#include "colour.h"

/*
** {===========================================================================
** Shapes
*/

namespace Shapes {

typedef std::function<void(int, int)> Plotter;
typedef std::function<int(int, int)> Indexer;
typedef std::function<Colour(int, int)> Picker;

int line(int x0, int y0, int x1, int y1, Plotter plot);

int box(int x0, int y0, int x1, int y1, Plotter plot);
int boxFill(int x0, int y0, int x1, int y1, Plotter plot);

int ellipse(int x, int y, int rx, int ry, Plotter plot);
int ellipseFill(int x, int y, int rx, int ry, Plotter plot);

int fill(int x, int y, int w, int h, int* rx0, int* ry0, int* rx1, int* ry1, int oldColor, int newColor, Indexer get, Plotter plot);
int fill(int x, int y, int w, int h, int* rx0, int* ry0, int* rx1, int* ry1, const Colour &oldColor, const Colour &newColor, Picker get, Plotter plot);

int replace(int x, int y, int w, int h, int* rx0, int* ry0, int* rx1, int* ry1, int oldColor, int newColor, Indexer get, Plotter plot);
int replace(int x, int y, int w, int h, int* rx0, int* ry0, int* rx1, int* ry1, const Colour &oldColor, const Colour &newColor, Picker get, Plotter plot);

}

/* ===========================================================================} */

#endif /* __SHAPES_H__ */
