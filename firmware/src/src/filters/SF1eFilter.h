/**
 One Euro Filter, C version
 
 Copyright (c) 2014 Jonathan Aceituno <join@oin.name>

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
 
 For details, see http://www.lifl.fr/~casiez/1euro
 */

#ifndef __SF1eFilter__
#define __SF1eFilter__

/**
 The state data for a simple low pass filter.
 */
typedef struct {
	float hatxprev;
	float xprev;
	char usedBefore;
} SFLowPassFilter;

/**
 The configuration for an instance of a 1 Euro Filter.
 */
typedef struct {
	float frequency;
	float minCutoffFrequency;
	float cutoffSlope;
	float derivativeCutoffFrequency;
} SF1eFilterConfiguration;

/**
 A 1 Euro Filter with configuration and state data.
 
 Example 1: Heap allocation, using a configuration struct, and a fixed frequency.
	SF1eFilterConfiguration config;
	config.frequency = 120;
	config.cutoffSlope = 1;
	config.derivativeCutoffFrequency = 1;
	config.minCutoffFrequency = 1;
	SF1eFilter *filter = SF1eFilterCreateWithConfig(config);
	// ...
	float filtered = SF1eFilterDo(filter, x);
	// ...
	filter = SF1eFilterDestroy(filter);
 
 Example 2: Heap allocation, using a variable frequency, without configuration struct.
	SF1eFilter *filter = SF1eFilterCreate(120, 1, 1, 1);
	// ...
	float time = // ... (get current timestamp)
	float filtered = SF1eFilterDoAtTime(filter, x, time);
	// ...
	filter = SF1eFilterDestroy(filter);
 
 Example 3: Stack allocation.
	SF1eFilter filter;
	filter.config.frequency = 120;
	filter.config.cutoffSlope = 1;
	filter.config.derivativeCutoffFrequency = 1;
	filter.config.minCutoffFrequency = 1;
	SF1eFilterInit(&filter);
	// ...
	float filtered = SF1eFilterDo(filter, x);
	// ...
	// (filter goes away with stack)
 */
typedef struct {
	SF1eFilterConfiguration config;
	SFLowPassFilter xfilt;
	SFLowPassFilter dxfilt;
	double lastTime;
	float frequency;
} SF1eFilter;


/**
 Allocate and initialize a new 1 Euro Filter with the given properties.
 */
SF1eFilter *SF1eFilterCreate(float frequency, float minCutoffFrequency, float cutoffSlope, float derivativeCutoffFrequency);
/**
 Allocate and initialize a new 1 Euro Filter with the given configuration.
 */
SF1eFilter *SF1eFilterCreateWithConfig(SF1eFilterConfiguration config);
/**
 Destroy a given 1 Euro Filter instance and free the associated memory resources.
 */
SF1eFilter *SF1eFilterDestroy(SF1eFilter *filter);
/**
 Initialize a 1 Euro Filter instance. Make sure to do this on a stack-allocated One Euro Filter before using it.
 */
void SF1eFilterInit(SF1eFilter *filter);
/**
 Filter a float using the given One Euro Filter.
 */
float SF1eFilterDo(SF1eFilter *filter, float x);
/**
 Filter a float using the given One Euro Filter and the given timestamp. Frequency will be automatically recomputed.
 */
float SF1eFilterDoAtTime(SF1eFilter *filter, float x, double timestamp);
/**
 Compute Alpha for a given One Euro Filter and a given cutoff frequency.
 */
float SF1eFilterAlpha(SF1eFilter *filter, float cutoff);
/**
 Test this One Euro Filter implementation and print test data to the standard output.
 You can then compare this output to the official implementation at http://www.lifl.fr/~casiez/1euro .
 */
void SF1eFilterTest();

/**
 Create a simple low-pass filter instance.
 */
SFLowPassFilter *SFLowPassFilterCreate();
/**
 Destroy a low-pass filter instance and free the associated memory resources.
 */
SFLowPassFilter *SFLowPassFilterDestroy(SFLowPassFilter *filter);
/**
 Initialize a low-pass filter instance. Make sure to do this on a stack-allocated low-pass filter instance before using it.
 */
void SFLowPassFilterInit(SFLowPassFilter *filter);
/**
 Filter a float using the given low-pass filter and the given alpha value.
 */
float SFLowPassFilterDo(SFLowPassFilter *filter, float x, float alpha);


#endif
