/*
 * GCQ-Switch gpio switch driver
 *
 * Copyright 2017 GCQ
 *
 * Based on Xilinx GPIO driver
 * Copyright 2008 Xilinx, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/init.h>
#include <linux/errno.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/of_gpio.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/module.h>
#include <linux/slab.h>

struct gcq_gpio_switch_chip {
	struct of_mm_gpio_chip mmchip;
	spinlock_t gpio_lock;	/* Lock used for synchronization */
};

static int gcq_gpio_switch_get(struct gpio_chip *gc, unsigned int gpio)
{
	int bank;
	int g;

	struct of_mm_gpio_chip *mm_gc = to_of_mm_gpio_chip(gc);

	bank = gpio >> 3;
	g = gpio%8;
  printk(KERN_INFO "gcq_gpio_get\n");
	return (ioread8(mm_gc->regs + bank) >> g) & 1;
}


/*
status = chip->get_direction(chip, offset);
	if (status > 0) {
		//GPIOF_DIR_IN, or other positive
		status = 1;
		clear_bit(FLAG_IS_OUT, &desc->flags);
	}
	if (status == 0) {
		// GPIOF_DIR_OUT
		set_bit(FLAG_IS_OUT, &desc->flags);
	}
*/
static int gcq_gpio_switch_get_direction(struct gpio_chip *gc, unsigned int gpio)
{
	printk(KERN_INFO "gcq_gpio_get_dir\n");
	return 1;
}


 static int gcq_gpio_switch_probe(struct platform_device *pdev)
 {
 	struct device_node *node = pdev->dev.of_node;
 	int status;
  const u32 *tree_info;
 	struct gcq_gpio_switch_chip *gcq_gc;

 	gcq_gc = devm_kzalloc(&pdev->dev, sizeof(*gcq_gc), GFP_KERNEL);
 	if (!gcq_gc)
 		return -ENOMEM;

 	spin_lock_init(&gcq_gc->gpio_lock);

  gcq_gc->mmchip.gc.ngpio = 8;
	tree_info = of_get_property(node, "xlnx,gpio-width", NULL);
	if (!tree_info)
		tree_info = of_get_property(node->parent,
					    "xlnx,gpio-width", NULL);
	if (tree_info)
		gcq_gc->mmchip.gc.ngpio = *tree_info;

	gcq_gc->mmchip.gc.get_direction = gcq_gpio_switch_get_direction;
	gcq_gc->mmchip.gc.get = gcq_gpio_switch_get;


	/* Call the OF gpio helper to setup and register the GPIO device */
	status = of_mm_gpiochip_add(node, &gcq_gc->mmchip);

  if (status) {
		dev_err(&pdev->dev, "Failed adding memory mapped gpiochip\n");
		return status;
	}

	platform_set_drvdata(pdev, gcq_gc);

  return status;

 }

static int gcq_gpio_switch_remove(struct platform_device *pdev)
{
	struct gcq_gpio_switch_chip *gcq_gc = platform_get_drvdata(pdev);

	of_mm_gpiochip_remove(&gcq_gc->mmchip);

	return 0;
}

static struct of_device_id gcq_gpio_switch_of_match[] = {
	{ .compatible = "dte,gcq-gpio-switch" },
	{ /* end of list */ },
};

MODULE_DEVICE_TABLE(of, gcq_gpio_switch_of_match);

static struct platform_driver gcq_gpio_switch_driver = {
	.driver = {
		.name	= "gcq-gpio-switch",
		.of_match_table = of_match_ptr(gcq_gpio_switch_of_match),
	},
	.probe		= gcq_gpio_switch_probe,
	.remove		= gcq_gpio_switch_remove,
};

static int __init gcq_gpio_switch_init(void)
{
	return platform_driver_register(&gcq_gpio_switch_driver);
}
subsys_initcall(gcq_gpio_switch_init);

static void __exit gcq_gpio_switch_exit(void)
{
	platform_driver_unregister(&gcq_gpio_switch_driver);
}
module_exit(gcq_gpio_switch_exit);

MODULE_AUTHOR("German CQ");
MODULE_DESCRIPTION("Testing GPIO SWITCH driver");
MODULE_LICENSE("GPL");
