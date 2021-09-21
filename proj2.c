#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/xarray.h>
#include <linux/radix-tree.h>
#include <linux/hashtable.h>
#include <linux/seq_file.h>

#define buffer_size 1000
#define hash_bits 10

/*
 *creates a table with myhash name and power of size 2 based on bits
 */
DEFINE_HASHTABLE(myhash,hash_bits);

/*
 *defines a new xarray with given name
 */

DEFINE_XARRAY(myxarray);

/*
 * initialise a new radix tree
 */

RADIX_TREE(myradix,GFP_KERNEL);

static char *int_str;


MODULE_LICENSE("GPL");
MODULE_AUTHOR("[Kashi Vishwanath]");
MODULE_DESCRIPTION("LKP Project 2");


module_param(int_str, charp, S_IRUSR | S_IRGRP | S_IROTH);


MODULE_PARM_DESC(int_str, "A comma-separated list of integers");


static LIST_HEAD(mylist);

static struct proc_dir_entry *ent;
struct rb_root mytree = RB_ROOT;
static unsigned long xarray_count=0;
static unsigned long radix_count=0;

struct entry {
	int val;
	struct list_head list;
	/*
	 * table is implemented using buckets containing a kernel struct hlist_head type
	 */
	struct hlist_node myhash_node;
	struct rb_node my_rb_node;
	struct xarray my_xarray;
	struct radix_tree_node my_radix_node;
};


/*
 * hello_proc_show decides the output
 * from recommended reading
 */
static int hello_proc_show(struct seq_file *m, void *v){
	seq_printf(m, "Hello proc!\n");
	return 0;
}

/*
  * hello_proc_open() is called when the proc file is opened. single_open() makes all the
  * data as output at once
  */

static int hello_proc_open(struct inode *inode, struct file *file){
	return single_open(file, hello_proc_show, NULL);
}


static ssize_t read_proc(struct file *file,char __user *ubuf,size_t count, loff_t *ppos){
	char buf[buffer_size];
	int len=0;
	int bkt;
	unsigned long xa_count, r_count;
	struct entry *node = NULL;
	struct rb_node *rbnode = NULL;
	if(*ppos > 0 || count < buffer_size)
		return 0;
	len += sprintf(buf,"Linked List: ");
        list_for_each_entry(node,&mylist,list){
		len += sprintf(buf+len,"%d,",node->val);
        }
	len += sprintf(buf+len-1,"\n");
	len += sprintf(buf+len,"Hash Table: ");
	hash_for_each(myhash,bkt,node,myhash_node){
		len += sprintf(buf+len,"%d,",node->val);
	}
	len += sprintf(buf+len-1,"\n");
	len += sprintf(buf+len,"Red-Black Tree: ");
	for (rbnode = rb_first(&mytree);rbnode;rbnode=rb_next(rbnode))
		len += sprintf(buf+len, "%d,",rb_entry(rbnode,struct entry, my_rb_node)->val);

	len += sprintf(buf+len-1,"\n");
	len += sprintf(buf+len,"Radix Tree: ");
	for (r_count=0;r_count<radix_count;r_count++){
		node = radix_tree_lookup(&myradix,r_count);
		len += sprintf(buf+len,"%d,",node->val);
	}

	len += sprintf(buf+len-1,"\n");
	len += sprintf(buf+len,"XArray: ");
	xa_for_each(&myxarray,xa_count,node) {
	       len += sprintf(buf+len,"%d,",node->val);
	}
	len += sprintf(buf+len-1,"\n");
	if(copy_to_user(ubuf, buf, len))
		return -EFAULT;
	*ppos = len;
	return len;
}

static const struct proc_ops hello_proc_fops = {
	.proc_open = hello_proc_open,
	.proc_read = read_proc,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};

void rb_insert(struct rb_root *myroot, struct entry *new_entry){
	struct rb_node **link = &myroot->rb_node;
	struct rb_node *parent = NULL;
	int value = new_entry->val;
	
	while (*link){
		parent = *link;
		struct entry *temp_entry = rb_entry(parent, struct entry, my_rb_node);
		if (temp_entry->val > value){
			link = &(*link)->rb_left;
		}else {
			link = &(*link)->rb_right;
		}
	}

	rb_link_node(&new_entry->my_rb_node, parent, link);
	rb_insert_color(&new_entry->my_rb_node, myroot);
}

void rt_insert(struct entry *new_entry){
	radix_tree_preload(GFP_KERNEL);
	radix_tree_insert(&myradix,radix_count,new_entry);
	radix_tree_preload_end();
	radix_count++;
}






static int store_value(int val)
{       
	struct entry *temp_node = NULL;
	int err;
	temp_node = kmalloc(sizeof(struct entry), GFP_KERNEL);
	if(temp_node == NULL){return -ENOMEM;}
	temp_node->val = val;
	INIT_LIST_HEAD(&temp_node->list);
	list_add_tail(&temp_node->list, &mylist);
	hash_add(myhash,&temp_node->myhash_node,temp_node->val);
	rb_insert(&mytree,temp_node);
	err = xa_err(xa_store(&myxarray,xarray_count,temp_node, GFP_KERNEL));
	if(!err){
	xarray_count++;}
	rt_insert(temp_node);
	return 0;

}

static void test_linked_list(void)
{
	struct entry *temp;
	struct rb_node *rbnode = NULL;
	unsigned long xa_count,r_count;
	int bkt;
        printk(KERN_INFO "Linked List: ");

        /*Traversing Linked List and Print its Members*/
        list_for_each_entry(temp, &mylist, list) {
        printk(KERN_CONT "%d ", temp->val);
        }

	printk(KERN_INFO "Hash Table: ");
	hash_for_each(myhash,bkt,temp,myhash_node){
	printk(KERN_CONT "%d ",temp->val);}

	/* Red Black tree */
	printk(KERN_INFO "Red-Black Tree: ");
	for(rbnode = rb_first(&mytree);rbnode;rbnode = rb_next(rbnode)){
		printk(KERN_CONT "%d ",rb_entry(rbnode,struct entry,my_rb_node)->val);
	}
	
	/* Radix Tree */
	printk(KERN_INFO "Radix Tree: ");
	for (r_count =0;r_count<radix_count;r_count++){
		temp = radix_tree_lookup(&myradix,r_count);
		printk(KERN_CONT "%d ",temp->val);
	}

	/* XARRAY */
	printk(KERN_INFO "XArray: ");
	xa_for_each(&myxarray,xa_count,temp){
		printk(KERN_CONT  "%d ",temp->val);
	}

}


static void destroy_linked_list_and_free(void)
{
	        /* Go through the list and free the memory. */
        struct entry *cursor, *temp;
	struct rb_node *rbnode = NULL;
	int bkt;
	unsigned long xa_count, r_count;
        list_for_each_entry_safe(cursor, temp, &mylist, list) {
            list_del(&cursor->list);
            kfree(cursor);
        }

	hash_for_each(myhash,bkt,temp,myhash_node){
		hash_del(&temp->myhash_node);
	}

	for (rbnode = rb_first(&mytree);rbnode;rbnode=rb_next(rbnode)){
		rb_erase(rbnode,&mytree);}
	
	for (r_count=0;r_count<radix_count;r_count++){
		radix_tree_delete(&myradix,r_count);}

	for (xa_count = 0; xa_count < xarray_count; xa_count++){
		xa_erase(&myxarray,xa_count);}

}
static int parse_params(void)
{
	int val, err = 0;
	char *p, *orig, *params;



	params = kstrdup(int_str, GFP_KERNEL);
	if (!params)
		return -ENOMEM;
	orig = params;

	
	while ((p = strsep(&params, ",")) != NULL) {
		if (!*p)
			continue;
		
		err = kstrtoint(p, 0, &val);
		if (err)
			break;

		
		err = store_value(val);
		if (err)
			break;
	}

	
	kfree(orig);
	return err;
}

static void run_tests(void)
{
	
	test_linked_list();
}

static void cleanup(void)
{
	
	printk(KERN_INFO "\nCleaning up...\n");

	destroy_linked_list_and_free();
}

static int __init proj2_init(void)
{
	int err = 0;

	
	if (!int_str) {
		printk(KERN_INFO "Missing \'int_str\' parameter, exiting\n");
		return -1;
	}

	
	err = parse_params();
	if (err)
		goto out;

	
	run_tests();
	ent = proc_create("proj2",0744,NULL,&hello_proc_fops);
out:

	return err;
}

static void __exit proj2_exit(void)
{
	
	printk(KERN_INFO "Module exiting....\n");
	proc_remove(ent);
	cleanup();
	return;
}


module_init(proj2_init);


module_exit(proj2_exit);

