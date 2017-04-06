#include "stdafx.h"
#include "Paths.h"

using namespace std;

namespace PsiEngine {

#ifndef _DEPLOY
	string Paths::RES = "../../Resources/";
#else
	string Paths::RES = "Resources/";
#endif

}