/*
** GB BASIC
**
** Copyright (C) 2023-2026 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#ifndef __APPLICATION_H__
#define __APPLICATION_H__

#include "../gbbasic.h"

/*
** {===========================================================================
** Application
*/

class Application* createApplication(class Workspace* workspace, int argc, const char* argv[]);
void destroyApplication(class Application* &app);
bool updateApplication(class Application* app);
void pushApplicationEvent(class Application* app, int event, int code, int data0, int data1);

/* ===========================================================================} */

#endif /* __APPLICATION_H__ */
