

#ifndef CAN_DEV_H
#define CAN_DEV_H

#include <linux/can.h>
#include <linux/can/netlink.h>
#include <linux/can/error.h>

enum can_mode {
	CAN_MODE_STOP = 0,
	CAN_MODE_START,
	CAN_MODE_SLEEP
};

struct can_priv {
	struct can_device_stats can_stats;

	struct can_bittiming bittiming;
	struct can_bittiming_const *bittiming_const;
	struct can_clock clock;

	enum can_state state;
	u32 ctrlmode;
	u32 ctrlmode_supported;

	int restart_ms;
	struct timer_list restart_timer;

	int (*do_set_bittiming)(struct net_device *dev);
	int (*do_set_mode)(struct net_device *dev, enum can_mode mode);
	int (*do_get_state)(const struct net_device *dev,
			    enum can_state *state);
	int (*do_get_berr_counter)(const struct net_device *dev,
				   struct can_berr_counter *bec);

	unsigned int echo_skb_max;
	struct sk_buff **echo_skb;
};

#define get_can_dlc(i)	(min_t(__u8, (i), 8))

/* Drop a given socketbuffer if it does not contain a valid CAN frame. */
static inline int can_dropped_invalid_skb(struct net_device *dev,
					  struct sk_buff *skb)
{
	const struct can_frame *cf = (struct can_frame *)skb->data;

	if (unlikely(skb->len != sizeof(*cf) || cf->can_dlc > 8)) {
		kfree_skb(skb);
		dev->stats.tx_dropped++;
		return 1;
	}

	return 0;
}

struct net_device *alloc_candev(int sizeof_priv, unsigned int echo_skb_max);
void free_candev(struct net_device *dev);

int open_candev(struct net_device *dev);
void close_candev(struct net_device *dev);

int register_candev(struct net_device *dev);
void unregister_candev(struct net_device *dev);

int can_restart_now(struct net_device *dev);
void can_bus_off(struct net_device *dev);

void can_put_echo_skb(struct sk_buff *skb, struct net_device *dev,
		      unsigned int idx);
void can_get_echo_skb(struct net_device *dev, unsigned int idx);
void can_free_echo_skb(struct net_device *dev, unsigned int idx);

struct sk_buff *alloc_can_skb(struct net_device *dev, struct can_frame **cf);
struct sk_buff *alloc_can_err_skb(struct net_device *dev,
				  struct can_frame **cf);

#endif /* CAN_DEV_H */