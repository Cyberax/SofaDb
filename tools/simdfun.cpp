#include <string>

#include <xmmintrin.h>

int main()
{
	const char *text="Hello, so very cruel and uncaring world. "
			"Yeah, I feel depressed today. Text to parse.";

	__m128i tb = _mm_loadu_si128((const __m128i *)text);


	return 0;
}
