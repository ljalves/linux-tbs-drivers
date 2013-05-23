#include <linux/slab.h>

void *ttkern_kzalloc(int size)
{
       return kzalloc(size, GFP_KERNEL);
}

void ttkern_kfree(void *p)
{
       kfree(p);
}
