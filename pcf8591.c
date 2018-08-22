/*
 * Copyright (c) 2018 Florian Scholz and Jennifer Gommans <florian90@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of mosquitto nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */


#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/uaccess.h>
#include <asm/uaccess.h>
#include <asm/delay.h>

#define DISABLE_WD

static dev_t pcf8591_dev_number;
static struct cdev *driver_object;
static struct class *pcf8591_class;
static struct device *pcf8591a_dev;
static struct device *pcf8591b_dev;
static struct i2c_adapter *adapter;
static struct i2c_client *slave_01, *slave_02;

struct instance_data {
	struct i2c_client *client;
};

static struct i2c_device_id pcf8591_idtable[] = {
	{ "pcf8591", 0 }, { "pcf8591a", 0 }, {}
};
MODULE_DEVICE_TABLE(i2c, pcf8591_idtable);

static struct i2c_board_info info_20 = {
	I2C_BOARD_INFO("pcf8591", 0x48), // 0x90 >> 1
};
static struct i2c_board_info info_30 = {
	//I2C_BOARD_INFO("pcf8591a", 0x92>>1), // 0x4a
	I2C_BOARD_INFO("pcf8591a", 0x49), // 0x4a
};

static void drive_reset(struct work_struct *data)
{
        char send_buffer[2];

        printk("drive_reset is called\n");
	send_buffer[0] = 0x40;
	send_buffer[1] = 0x80;
	printk("send %x %x addr: %x\n", send_buffer[0],
                send_buffer[1], slave_01->addr);
	i2c_master_send(slave_01, send_buffer, sizeof(send_buffer));

	printk("send %x %x addr: %x\n", send_buffer[0],
                send_buffer[1], slave_02->addr);
	i2c_master_send(slave_02, send_buffer, sizeof(send_buffer));
} 

DECLARE_DELAYED_WORK( robo_watchdog, drive_reset );

static int driver_open( struct inode *geraetedatei, struct file *instanz )
{
	struct instance_data *instance_data =
		kmalloc( sizeof(struct instance_data), GFP_USER );
	printk("%d\n", iminor(geraetedatei));
	if(iminor(geraetedatei) == 0) {
		printk("open dac-1\n");
		instance_data->client = slave_01;
	}
	else if(iminor(geraetedatei) == 1) {
		printk("open dac-2\n");
		instance_data->client = slave_02;
	} else {
		return -EIO;
	}
	instanz->private_data = instance_data;
	return 0;
}
static int driver_close( struct inode *geraete_datei, struct file *instanz )
{
	printk("close something\n");
	kfree(instanz->private_data);
	return 0;
}


static ssize_t driver_write( struct file *instanz, const char __user *user,
  size_t count, loff_t *offset )
{

	char send_buffer[2];
	unsigned long not_copied, to_copy;
	struct instance_data *id;

	not_copied = 0, to_copy = count;

	send_buffer[0] = 0x40;
	to_copy = min( count, sizeof(send_buffer)-1);
	not_copied = copy_from_user(&send_buffer[1], user, to_copy);

	if (send_buffer[1]==0) {
		return count;
	}

	id=(struct instance_data *)instanz->private_data;

	printk("send %x %x addr: %x\n", send_buffer[0],
		send_buffer[1], id->client->addr);
	i2c_master_send(id->client, send_buffer, sizeof(send_buffer));
	
	cancel_delayed_work( &robo_watchdog );

	if (schedule_delayed_work( &robo_watchdog, 1*HZ )==0) {
		printk("schedule_delayed_work failed...\n");
	} else {
		printk("watchdog activated...\n");
	}
	
	return to_copy-not_copied;
}


static int pcf8591_probe( struct i2c_client *client,
	const struct i2c_device_id *id )
{
	unsigned char buf[2];

	printk("pcf8591_probe: %p %p \"%s\"- ",client,id,id->name);
	printk("slaveaddr: %d, name: %s\n",client->addr,client->name);

	if (client->addr != 0x48 && client->addr != 0x49 ) {
		printk("i2c_probe %x: not found\n",client->addr);
		return -1;
	}
	
	buf[0] = 0x40;
	buf[1] = 0x80;
	i2c_master_send( client, buf, 2 );
	return 0;
}

static int pcf8591_remove( struct i2c_client *client )
{
	return 0;
}
static struct file_operations fops = {
	.owner= THIS_MODULE,
	.write = driver_write,
	.open = driver_open,
	.release = driver_close
};

static struct i2c_driver pcf8591_driver = {
	.driver = {
		.name   = "pcf8591",
	},
	.id_table       = pcf8591_idtable,
	.probe          = pcf8591_probe,
	.remove         = pcf8591_remove,
};

static int __init mod_init( void )
{
	if( alloc_chrdev_region(&pcf8591_dev_number,0,2,"pcf8591")<0 )
		return -EIO;
	
	driver_object = cdev_alloc();
	if( driver_object==NULL )
		goto free_device_number;
	driver_object->owner = THIS_MODULE;
	driver_object->ops = &fops;
	
	if( cdev_add(driver_object,pcf8591_dev_number,1) )
		goto free_cdev;

	if( cdev_add(driver_object,pcf8591_dev_number+1,1) )
		goto free_cdev;
	pcf8591_class = class_create( THIS_MODULE, "pcf8591" );
	
	if( IS_ERR( pcf8591_class ) ) {
		pr_err( "pcf8591: no udev support\n");
		goto free_cdev;
	}
	
	pcf8591a_dev = device_create( pcf8591_class, NULL,
	pcf8591_dev_number, NULL, "%s", "dac-1" );
	if(pcf8591a_dev == NULL) 
	{
		pr_err("device_create(): epic fail\n");
		goto destroy_dev_class;
	}
	
	dev_info(pcf8591a_dev, "mod_init\n");

	pcf8591b_dev = device_create( pcf8591_class, NULL,
	pcf8591_dev_number+1, NULL, "%s", "dac-2" );
	if(pcf8591b_dev == NULL) {
		pr_err("device_create(): epic fail\n");
		goto destroy_dev_class;
	}
	dev_info(pcf8591b_dev, "mod_init\n");
	
	if (i2c_add_driver(&pcf8591_driver)) {
		pr_err("i2c_add_driver failed\n");
		goto destroy_dev_class;
	}
	
	adapter = i2c_get_adapter(1); // Adapter sind durchnummeriert
	if (adapter==NULL) {
		pr_err("i2c_get_adapter failed\n");
		goto del_i2c_driver;
	}
	
	slave_01 = i2c_new_device( adapter, &info_20 );
	if (slave_01==NULL) {
		pr_err("i2c_new_device failed\n");
		goto del_i2c_driver;
	}

	slave_02 = i2c_new_device( adapter, &info_30 );

	if(slave_02 == NULL) {
		pr_err("i2c_new_device failed\n");
		goto del_i2c_driver;
	}
	return 0;
del_i2c_driver:
	i2c_del_driver(&pcf8591_driver);
destroy_dev_class:
	device_destroy( pcf8591_class, pcf8591_dev_number );
	class_destroy( pcf8591_class );
free_cdev:
	kobject_put( &driver_object->kobj );
free_device_number:
	unregister_chrdev_region( pcf8591_dev_number, 1 );
	return -EIO;
}

static void __exit mod_exit( void )
{
	dev_info(pcf8591a_dev, "mod_exit");
	dev_info(pcf8591b_dev, "mod_exit");
	device_destroy( pcf8591_class, pcf8591_dev_number );
	device_destroy( pcf8591_class, pcf8591_dev_number+1);
	class_destroy( pcf8591_class );
	cdev_del( driver_object );
	unregister_chrdev_region( pcf8591_dev_number, 1 );
	flush_scheduled_work();
	i2c_unregister_device(slave_01);
	i2c_unregister_device(slave_02);
	i2c_del_driver(&pcf8591_driver);
	return;
}

module_init( mod_init );
module_exit( mod_exit );
MODULE_LICENSE("GPL");
