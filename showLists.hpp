#ifndef _SHOW_LIST_HPP
#define _SHOW_LIST_HPP

namespace showlist
{

	typedef struct showEffect ShowEffect;

	extern const int maxNumberEffects;
	extern ShowEffect showList[];

	typedef struct showEffect {
		char effectName[18];
	} ShowEffect;
}
#endif

