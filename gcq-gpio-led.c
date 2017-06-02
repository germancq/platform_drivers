/*
 * JB-Trivial gpio driver
 *
 * Copyright 2010 Jonas Bonn
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

struct gcq_gpio_led_chip {
	struct of_mm_gpio_chip mmchip;
	spinlock_t gpio_lock;	/* Lock used for synchronization */
};

static int gcq_gpio_led_get(struct gpio_chip *gc, unsigned int gpio)
{
	int bank;
	int g;

	struct of_mm_gpio_chip *mm_gc = to_of_mm_gpio_chip(gc);

	bank = gpio >> 3;
	g = gpio%8;
  printk(KERN_INFO "gcq_gpio_get\n");
	return (ioread8(mm_gc->regs + bank) >> g) & 1;
}

static void gcq_gpio_led_set(struct gpio_chip *gc, unsigned int gpio, int val)
{

	unsigned long flags;
	struct of_mm_gpio_chip *mm_gc = to_of_mm_gpio_chip(gc);
	int bank;
	int g;
  unsigned int data_reg;

  struct gcq_gpio_led_chip *gcq_gc =
	    container_of(mm_gc, struct gcq_gpio_led_chip, mmchip);

	bank = gpio >> 3;
	g = gpio%8;


	spin_lock_irqsave(&gcq_gc->gpio_lock, flags);

  data_reg = ioread8(mm_gc->regs + bank);

	/* Write to GPIO signal and set its direction to output */
	if (val)
		data_reg |= 1 << g;
	else
		data_reg &= ~(1 << g);

	iowrite8(data_reg, mm_gc->regs + bank);

	spin_unlock_irqrestore(&gcq_gc->gpio_lock, flags);
  printk(KERN_INFO "gcq_gpio_set\n");
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
static int gcq_gpio_led_get_direction(struct gpio_chip *gc, unsigned int gpio)
{
	printk(KERN_INFO "gcq_gpio_get_dir\n");
	return 0;
}

static void gcq_gpio_led_save_regs(struct of_mm_gpio_chip *mm_gc)
{

  int j;
  int i = mm_gc->gc.ngpio;
	i = (i-1)/8;
  printk(KERN_INFO "gcq_gpio_led_save_regs with i = %d\n",i);
	for (j = 0; j < i+1; j++) {
		iowrite8(0xAA, mm_gc->regs + j);
	}
}

 static int gcq_gpio_led_probe(struct platform_device *pdev)
 {
 	struct device_node *node = pdev->dev.of_node;
 	int status;
  const u32 *tree_info;
 	struct gcq_gpio_led_chip *gcq_gc;

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

	gcq_gc->mmchip.gc.get_direction = gcq_gpio_led_get_direction;
	gcq_gc->mmchip.gc.get = gcq_gpio_led_get;
	gcq_gc->mmchip.gc.set = gcq_gpio_led_set;

	gcq_gc->mmchip.save_regs = gcq_gpio_led_save_regs;

	/* Call the OF gpio helper to setup and register the GPIO device */
	status = of_mm_gpiochip_add(node, &gcq_gc->mmchip);

  if (status) {
		dev_err(&pdev->dev, "Failed adding memory mapped gpiochip\n");
		return status;
	}

	platform_set_drvdata(pdev, gcq_gc);

  return status;

 }

static int gcq_gpio_led_remove(struct platform_device *pdev)
{
	struct gcq_gpio_led_chip *gcq_gc = platform_get_drvdata(pdev);

	of_mm_gpiochip_remove(&gcq_gc->mmchip);

	return 0;
}

static struct of_device_id gcq_gpio_led_of_match[] = {
	{ .compatible = "dte,gcq-gpio-led" },
	{ /* end of list */ },
};

MODULE_DEVICE_TABLE(of, gcq_gpio_led_of_match);

static struct platform_driver gcq_gpio_led_driver = {
	.driver = {
		.name	= "gcq-gpio-led",
		.of_match_table = of_match_ptr(gcq_gpio_led_of_match),
	},
	.probe		= gcq_gpio_led_probe,
	.remove		= gcq_gpio_led_remove,
};

static int __init gcq_gpio_led_init(void)
{
	return platform_driver_register(&gcq_gpio_led_driver);
}
subsys_initcall(gcq_gpio_led_init);

static void __exit gcq_gpio_led_exit(void)
{
	platform_driver_unregister(&gcq_gpio_led_driver);
}
module_exit(gcq_gpio_led_exit);

MODULE_AUTHOR("German CQ");
MODULE_DESCRIPTION("Testing GPIO LED driver");
MODULE_LICENSE("GPL");
