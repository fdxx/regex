#include "../regex.hpp"
#include <cstdio>

int main()
{
	const char *str = "a,b,c,d,b,c";

	fdxx::Regex reg(",(b),");
	int matches = reg.MatchAll(str);
	
	for (int i = 0; i < matches; i++)
	{
		printf("--- matches %i ---\n", i);
		for (int j = 0; j < reg.CaptureCount(i); j++)
		{
			char *buffer = reg.GetSubString(j, i);
			printf("[%i]%s|MatchOffset = %i(%i)\n", j, buffer, reg.MatchOffset(MATCHOFFSET_START, i), reg.MatchOffset(MATCHOFFSET_END, i));
			delete[] buffer;
		}
	}
}
