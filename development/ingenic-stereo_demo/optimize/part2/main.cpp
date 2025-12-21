#include "forward.h"

#include <StereoDisparity.hpp>
#include <stdint.h>
#include <limits.h>
#include <string.h>
#include <saturate_cast.hpp>


int act_bitwidth = 1;
FUNC A::test = NULL;
FUNC B::test = NULL;
FUNC C::test = NULL;

void RegisterA(FUNC a)
{
	A::test = a;
	if(!B::test)
		B::test = a;
}
void RegisterB(FUNC b)
{
	B::test = b;
}

void RegisterC(FUNC c)
{
	C::test = c;
}


#define TESTCOUNT 		1

int main(int argc, char **argv)
{
  long long st,diffA,diffB, diffC;

	A a(0);
	B b(0);
	// C c(0);
	a.Init();
	b.Init(&a);
	// c.Init(&a);

	printf("----- MSA ---- \n");
	st = A::getSystemTime();
	for(int i = 0;i < TESTCOUNT;i++)
	    b.Test();
	diffB = A::getSystemTime() - st;
	printf("OTest Time = %lld\n",diffB);

	printf("----- C ---- %d \n", TESTCOUNT);
	st = A::getSystemTime();
	for(int i = 0;i < TESTCOUNT;i++)
	  a.Test();
	diffA = A::getSystemTime() - st;
	printf("cTest Time = %lld\n",diffA);

	// printf("----- MSA 2---- \n");
	// st = A::getSystemTime();
	// for(int i = 0;i < TESTCOUNT;i++)
	//     c.Test();
	// diffC = A::getSystemTime() - st;
	// printf("OTest Time = %lld\n",diffC);
	// if(diffC > 0)
	//   printf("Optimization rate = %.02f\n",(float)(diffA - diffC) / diffC);
	// a.CheckData(&c);

	if(diffB > 0)
	  printf("Optimization rate = %.02f\n",(float)(diffA - diffB) / diffB);
	a.CheckData(&b);
	printf("test Ok.\n");
}
