#include <linux/kernel.h>

#include <common.h>


static void __init ayhd_init (void)
{
	printk(KERN_INFO "ayh driver started.\n");

}

static void __exit ayhd_release (void)
{
	printk(KERN_INFO "ayh driver released.\n");

}


module_init(ayhd_init);
module_exit(ayhd_release);
