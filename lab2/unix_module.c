#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/net.h>
#include <net/sock.h>
#include <net/tcp_states.h>
#include <net/af_unix.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#define BUCKET_SPACE (BITS_PER_LONG - (UNIX_HASH_BITS + 1) - 1)

#define get_bucket(x) ((x) >> BUCKET_SPACE)
#define get_offset(x) ((x) & ((1UL << BUCKET_SPACE) - 1))
#define set_bucket_offset(b, o) ((b) << BUCKET_SPACE | (o))

static const char *const type_names[] = {
	"DGRAM",
	"STREAM",
	"RAW",
	"RDM",
	"SEQPACKET",
	"DCCP",
	"UNKNOWN",
	"UNKNOWN",
	"UNKNOWN",
	"PACKET"
};

static struct sock *unix_from_bucket(struct seq_file *seq, loff_t *pos)
{
	unsigned long offset = get_offset(*pos);
	unsigned long bucket = get_bucket(*pos);
	unsigned long count = 0;
	struct sock *sk;

	for (sk = sk_head(&seq_file_net(seq)->unx.table.buckets[bucket]);
	     sk; sk = sk_next(sk)) {
		if (++count == offset)
			break;
	}

	return sk;
}

static struct sock *unix_get_first(struct seq_file *seq, loff_t *pos)
{
	unsigned long bucket = get_bucket(*pos);
	struct net *net = seq_file_net(seq);
	struct sock *sk;

	while (bucket < UNIX_HASH_SIZE) {
		spin_lock(&net->unx.table.locks[bucket]);

		sk = unix_from_bucket(seq, pos);
		if (sk)
			return sk;

		spin_unlock(&net->unx.table.locks[bucket]);

		*pos = set_bucket_offset(++bucket, 1);
	}

	return NULL;
}

static struct sock *unix_get_next(struct seq_file *seq, struct sock *sk,
				  loff_t *pos)
{
	unsigned long bucket = get_bucket(*pos);

	sk = sk_next(sk);
	if (sk)
		return sk;


	spin_unlock(&seq_file_net(seq)->unx.table.locks[bucket]);

	*pos = set_bucket_offset(++bucket, 1);

	return unix_get_first(seq, pos);
}

static int unix_seq_show(struct seq_file *seq, void *v) 
{
	if (v == SEQ_START_TOKEN) {
		seq_printf(seq, "%5s %-5s %-8s %-15s %-15s %-8s %s\n", "Proto", 
    "RefCnt", "Flags", "Type", "State", "I-Node", "Путь");
		return 0;
	}
	struct sock *s = v;
	struct unix_sock *u = unix_sk(s);
	unix_state_lock(s);
		
	seq_printf(seq, "%5s %-5d %-8s %-15s %-15s %-8ld", "unix", refcount_read(&s->sk_refcnt), 
			s->sk_state == TCP_LISTEN ? "ACC" : "",
			type_names[s->sk_type],
			s->sk_socket ? (s->sk_state == TCP_ESTABLISHED ? "CONNECTED" : "") :
			(s->sk_state == TCP_ESTABLISHED ? "CONNECTING" : "DISCONNECTING"),
			sock_i_ino(s));

	if (u->addr) {	
		int i, len;
		seq_putc(seq, ' ');

		i = 0;
		len = u->addr->len - offsetof(struct sockaddr_un, sun_path);
		if (u->addr->name->sun_path[0]) {
			len--;
		} else {
			seq_putc(seq, '@');
			i++;
		}
		for ( ; i < len; i++)
			seq_putc(seq, u->addr->name->sun_path[i] ?: '@');
		}
	unix_state_unlock(s);
	seq_putc(seq, '\n');
	
	return 0;
}

static void *unix_seq_start(struct seq_file *seq, loff_t *pos)
{
	if (!*pos)
		return SEQ_START_TOKEN;

	return unix_get_first(seq, pos);
}

static void *unix_seq_next(struct seq_file *seq, void *v, loff_t *pos)
{
	++*pos;

	if (v == SEQ_START_TOKEN)
		return unix_get_first(seq, pos);

	return unix_get_next(seq, v, pos);
}

static void unix_seq_stop(struct seq_file *seq, void *v)
{
	struct sock *sk = v;

	if (sk)
		spin_unlock(&seq_file_net(seq)->unx.table.locks[sk->sk_hash]);
}

static const struct seq_operations unixstat_seq_ops = {
	.start		= unix_seq_start,
	.next		= unix_seq_next,
	.stop		= unix_seq_stop,
	.show		= unix_seq_show,
};

static int __net_init knetstat_net_init(struct net *net) 
{
	if (!proc_create_net("unixstat", 0444, net->proc_net, &unixstat_seq_ops,
			sizeof(struct seq_net_private)))
		goto exit;

	return 0;

exit:
	return -ENOMEM;
}

static void __net_exit knetstat_net_exit(struct net *net) 
{
	remove_proc_entry("unixstat", net->proc_net);
}

static struct pernet_operations knetstat_net_ops = { 
	.init = knetstat_net_init,
	.exit = knetstat_net_exit, 
};

static int knetstat_init(void) 
{
	int err;

	err = register_pernet_subsys(&knetstat_net_ops);
	if (err < 0)
		return err;

	return 0;
}

static void knetstat_exit(void) 
{
	unregister_pernet_subsys(&knetstat_net_ops);
}

module_init(knetstat_init)
module_exit(knetstat_exit)

MODULE_LICENSE("GPL");

MODULE_AUTHOR("ExcaliBBur");

MODULE_DESCRIPTION("A Linux model designed for the second part of netstat utility"); 
