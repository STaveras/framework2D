// Hardware.h
// Information about the underlying system metal
// This is also me procrastinating on what I actually want to do

#pragma once

namespace framework
{
	namespace hardware
	{
		// TODO: Actually query the hardware for number of cores
		static size_t numComputeCores(void) {
			return 2; // by default, must have at least two... 
		}
	}
}