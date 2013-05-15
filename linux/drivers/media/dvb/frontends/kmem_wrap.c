#include <linux/slab.h>

void *kmalloc_wrap(int size)
{
       return kmalloc(size, GFP_KERNEL);
}

void *kzalloc_wrap(int size)
{
       return kzalloc(size, GFP_KERNEL);
}

void kfree_wrap(void *p)
{
       kfree(p);
}
