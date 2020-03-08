#pragma  once

#ifdef WIN32
	#ifdef MSPLAYER_EXPORTS
		#define MSPLAYER_API __declspec(dllexport)
	#else
		#define MSPLAYER_API __declspec(dllimport)
	#endif
#else
	#define MSPLAYER_API
#endif

