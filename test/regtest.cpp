#include "../regex.hpp"
#include <cstdio>

int main()
{
	const char *str = "a,b,c,d,b,c";

	Regex reg(",(b),");
	int matches = reg.MatchAll(str);
	char buffer[256];

	for (int i = 0; i < matches; i++)
	{
		printf("--- matches %i ---\n", i);
		for (int j = 0; j < reg.CaptureCount(i); j++)
		{
			int size = reg.GetSubString(j, buffer, 256, i);
			printf("[%i]%s|size = %i|MatchOffset = %i(%i)\n", j, buffer, size, reg.MatchOffset(i, 0), reg.MatchOffset(i, 1));
		}
	}
}
