#include <stdio.h>

int main(int argc,char **argv)
{
#ifdef __FAST_MATH__

 if(argc>1)
    printf("Compiled with -ffast-math => results may differ slightly.\n");

 return 0;

#else

 if(argc>1)
    printf("Not compiled with -ffast-math => results should match exactly.\n");

 return 1;

#endif
}
