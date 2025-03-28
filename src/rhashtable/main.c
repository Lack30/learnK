#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/rhashtable.h>
#include <linux/jhash.h>

MODULE_LICENSE("GPL");

struct hash_entry {
	struct rhash_head node;
	u32 key;
	u64 value;
};

void hash_entry_free(void *ptr, void *arg)
{
	kfree(ptr);
}

static int __init start_init(void)
{
	
	struct rhashtable_params param = {
		.key_len = sizeof(u32),
		.key_offset = offsetof(struct hash_entry, key),
		.head_offset = offsetof(struct hash_entry, node),
		.automatic_shrinking = true,
		.hashfn = jhash,
	};
	struct rhashtable rht;
	int ret = rhashtable_init(&rht, &param);
	printk("rhashtable_init returns %d\n", ret);
	if (ret < 0)
		return ret;

	u32 i;
	struct hash_entry *entry;
	for (i = 0; i < 1000; ++i) {
		entry = kzalloc(sizeof(struct hash_entry), GFP_KERNEL);
		if (entry == NULL) {
			printk("kzalloc returns NULL\n");
			goto err_exit;
		}
		entry->key = i;
		entry->value = (u64)i * i;
		//printk("Inserting %u %llu\n", entry->key, entry->value);
		ret = rhashtable_insert_fast(&rht, &entry->node, param);
		if (ret < 0) {
			kfree(entry);
			printk("rhashtable_insert_fast returns %d\n", ret);
			goto err_exit;
		}
	}
	for (i = 0; i < 1000; ++i) {
		// 如果可以保证entry不被删掉，那么可以用rhashtable_lookup_fast
		entry = rhashtable_lookup_fast(&rht, &i, param);
		if (entry == NULL) {
			printk("rhashtable_lookup_fast returns NULL\n");
			goto err_exit;
		}
		if (entry->value != (u64)i * i) {
			printk("%u %llu\n", i, entry->value);
			goto err_exit;
		}

		// 如果另一个线程可能会删掉entry，那么需要一直拿着read lock直到不再需要entry
		rcu_read_lock();
		entry = rhashtable_lookup(&rht, &i, param);
		if (entry == NULL) {
			rcu_read_unlock();
			printk("rhashtable_lookup returns NULL\n");
			goto err_exit;
		}
		if (entry->value != (u64)i * i) {
			rcu_read_unlock();
			printk("%u %llu\n", i, entry->value);
			goto err_exit;
		}
		rcu_read_unlock();
	}

	u32 key;
	key = 1;
	rcu_read_lock();
	entry = rhashtable_lookup(&rht, &key, param);
	if (entry == NULL) {
		rcu_read_unlock();
		printk("rhashtable_lookup returns NULL\n");
		goto err_exit;
	}
	rcu_read_unlock();
	pr_info("remove one entry key = %u value = %llu\n", key, entry->value);
	rhashtable_remove_fast(&rht, &entry->node, param);

err_exit:
	rhashtable_free_and_destroy(&rht, hash_entry_free, NULL);
	return 0;
}
module_init(start_init);

static void __exit end_exit(void)
{
}

module_exit(end_exit);