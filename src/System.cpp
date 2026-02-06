
#include "System.h"

const char* System::checkArgumentsForDataPath(int argc, const char** argv)
{
	if (argc > 2) {

		for (int i = 0; i < argc; i++) {
			if (!strcmp(argv[i], "--dataPath") || !strcmp(argv[i], "-d")) {
				return argv[i + 1]; // What if it's empty?
			}
		}
	}
	return "";
}
