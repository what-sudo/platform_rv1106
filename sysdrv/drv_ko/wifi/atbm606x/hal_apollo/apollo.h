/*
 * Common private data for altobeam APOLLO drivers
 *
 * Copyright (c) 2016, altobeam
 * Author:
 *
 *  Based on apollo  Copyright (c) 2010, ST-Ericsson
 * Author: Dmitry Tarnyagin <dmitry.tarnyagin@stericsson.com>
 *
 * Based on the mac80211 Prism54 code, which is
 * Copyright (c) 2006, Michael Wu <flamingice@sourmilk.net>
 *
 * Based on the islsm (softmac prism54) driver, which is:
 * Copyright 2004-2006 Jean-Baptiste Note <jbnote@gmail.com>, et al.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef ATBM_APOLLO_H
#define ATBM_APOLLO_H
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/mutex.h>
#include <linux/workqueue.h>
#include <linux/sched.h>
#include <linux/atomic.h>
#include <net/atbm_mac80211.h>
#include "apollo_plat.h"
#include "mac80211/atbm_common.h"
#define ATBM_WIFI_MAX_VIFS			(2)
#define ATBM_WIFI_GENERIC_IF_ID		(2)
#define ATBM_WIFI_HOST_VIF0_11N_THROTTLE	(ATBM_WIFI_MAX_QUEUE_SZ*3/4-1)
#define ATBM_WIFI_HOST_VIF1_11N_THROTTLE	(ATBM_WIFI_MAX_QUEUE_SZ*3/4-1)
#define ATBM_WIFI_HOST_VIF0_11BG_THROTTLE	(63)
#define ATBM_WIFI_HOST_VIF1_11BG_THROTTLE	(63)

#define RATE_INDEX_B_1M           0
#define RATE_INDEX_B_2M           1
#define RATE_INDEX_B_5_5M         2
#define RATE_INDEX_B_11M          3
#define RATE_INDEX_PBCC_22M       4     // not supported/unused
#define RATE_INDEX_PBCC_33M       5     // not supported/unused
#define RATE_INDEX_A_6M           6
#define RATE_INDEX_A_9M           7
#define RATE_INDEX_A_12M          8
#define RATE_INDEX_A_18M          9
#define RATE_INDEX_A_24M          10
#define RATE_INDEX_A_36M          11
#define RATE_INDEX_A_48M          12
#define RATE_INDEX_A_54M          13
#define RATE_INDEX_N_6_5M         14
#define RATE_INDEX_N_13M          15
#define RATE_INDEX_N_19_5M        16
#define RATE_INDEX_N_26M          17
#define RATE_INDEX_N_39M          18
#define RATE_INDEX_N_52M          19
#define RATE_INDEX_N_58_5M        20
#define RATE_INDEX_N_65M          21
#define RATE_INDEX_N_MCS32_6M        22
#define RATE_INDEX_HE_MCSER0         	23
#define RATE_INDEX_HE_MCSER1         	24
#define RATE_INDEX_HE_MCSER2         	25
#define RATE_INDEX_HE_MC0          	26
#define RATE_INDEX_HE_MC1          	27
#define RATE_INDEX_HE_MC2          	28
#define RATE_INDEX_HE_MC3          	29
#define RATE_INDEX_HE_MC4          	30
#define RATE_INDEX_HE_MC5          	31
#define RATE_INDEX_HE_MC6          	32
#define RATE_INDEX_HE_MC7          	33
#define RATE_INDEX_HE_MC8          	34
#define RATE_INDEX_HE_MC9          	35
#define RATE_INDEX_HE_MC10         	36
#define RATE_INDEX_HE_MC11         	37


#define RATE_INDEX_MAX         		23

#define ATBM_WIFI_MAX_QUEUE_SZ		(128)

#define ATBM_QUEUE_DEFAULT_CAP		(ATBM_WIFI_MAX_QUEUE_SZ>>4)
#define ATBM_QUEUE_SINGLE_CAP		(ATBM_WIFI_MAX_QUEUE_SZ*7/8)
#define ATBM_QUEUE_COMB_CAP			(ATBM_WIFI_MAX_QUEUE_SZ/2-8)


#define ST_PAYLOAD_BUF_SZ			(256)

#define IEEE80211_FCTL_WEP      0x4000
#define IEEE80211_QOS_DATAGRP   0x0080
#define WSM_KEY_MAX_IDX		20
struct hif_tx_encap;
struct wsm_tx_encap;
struct wsm_rx_encap;

#include "queue.h"
#include "wsm.h"
#include "scan.h"
#include "txrx.h"
#include "ht.h"
#include "pm.h"
#include "fwio.h"
//#ifdef CONFIG_ATBM_APOLLO_TESTMODE
#include "atbm_testmode.h"
//#endif /*CONFIG_ATBM_APOLLO_TESTMODE*/
#ifdef ATBM_SUPPORT_SMARTCONFIG
#include "smartconfig.h"
#endif

/* extern */ struct sbus_ops;
/* extern */ struct task_struct;
/* extern */ struct atbm_debug_priv;
/* extern */ struct atbm_debug_common;
/* extern */ struct firmware;

/* #define ROC_DEBUG */

/* hidden ssid is only supported when separate probe resp IE
   configuration is supported */
//#ifdef ATBM_PROBE_RESP_EXTRA_IE
#define HIDDEN_SSID	1
//#endif

#if defined(CONFIG_ATBM_APOLLO_TXRX_DEBUG)
#define txrx_printk(...) atbm_printk_always(__VA_ARGS__)
#else
#define txrx_printk(...)
#endif

#define ATBM_APOLLO_MAX_CTRL_FRAME_LEN	(0x1000)

#define ATBMWIFI_MAX_STA_IN_AP_MODE	(16)
#define WLAN_LINK_ID_MAX		(ATBMWIFI_MAX_STA_IN_AP_MODE + 3)
#define ATBM_UAPSD_VIRTUAL_LINKID					(WLAN_LINK_ID_MAX - 1)
#define ATBM_DTIM_VIRTUAL_LINKID					(WLAN_LINK_ID_MAX - 2)
static inline u32 atbm_uapsd_virtual_linkid(void)
{
	return ATBM_UAPSD_VIRTUAL_LINKID;
}
static inline u32 atbm_dtim_virtual_linkid(void)
{
	return ATBM_DTIM_VIRTUAL_LINKID; 
}

#define ATBM_APOLLO_MAX_STA_IN_AP_MODE		 ATBMWIFI_MAX_STA_IN_AP_MODE
#define ATBM_APOLLO_MAX_REQUEUE_ATTEMPTS	(5)
#define ATBM_APOLLO_LINK_ID_UNMAPPED		(15)

#define ATBM_APOLLO_MAX_TID			(8)

#define ATBM_APOLLO_TX_BLOCK_ACK_ENABLED_FOR_ALL_TID         (0xff)
#define ATBM_APOLLO_RX_BLOCK_ACK_ENABLED_FOR_ALL_TID         (0xff)
#define ATBM_APOLLO_RX_BLOCK_ACK_ENABLED_FOR_BE_TID \
	(ATBM_APOLLO_TX_BLOCK_ACK_ENABLED_FOR_ALL_TID & 0x01)
#define ATBM_APOLLO_TX_BLOCK_ACK_DISABLED_FOR_ALL_TID	(0)
#define ATBM_APOLLO_RX_BLOCK_ACK_DISABLED_FOR_ALL_TID	(0)

#define ATBM_APOLLO_BLOCK_ACK_CNT		(30)
#define ATBM_APOLLO_BLOCK_ACK_THLD		(800)
#define ATBM_APOLLO_BLOCK_ACK_HIST		(3)
#define ATBM_APOLLO_BLOCK_ACK_INTERVAL	(1 * HZ / ATBM_APOLLO_BLOCK_ACK_HIST)
#define ATBM_WIFI_ALL_IFS			(-1)
#ifdef CONFIG_ATBM_SUPPORT_SCHED_SCAN
#ifdef ROAM_OFFLOAD
#define ATBM_APOLLO_SCAN_TYPE_ACTIVE 0x1000
#define ATBM_APOLLO_SCAN_BAND_5G 0X2000
#endif /*ROAM_OFFLOAD*/
#endif
#define IEEE80211_FCTL_WEP      0x4000
#define IEEE80211_QOS_DATAGRP   0x0080
#ifdef CONFIG_ATBM_APOLLO_TESTMODE
#define ATBM_APOLLO_SCAN_MEASUREMENT_PASSIVE (0)
#define ATBM_APOLLO_SCAN_MEASUREMENT_ACTIVE  (1)
#endif

#ifdef MCAST_FWDING
#define WSM_MAX_BUF		30
#endif

/* Please keep order */
enum atbm_join_status {
	ATBM_APOLLO_JOIN_STATUS_PASSIVE = 0,	
	ATBM_APOLLO_JOIN_STATUS_STA_LISTEN,
	ATBM_APOLLO_JOIN_STATUS_MONITOR,
	ATBM_APOLLO_JOIN_STATUS_IBSS,
	ATBM_APOLLO_JOIN_STATUS_STA,
	ATBM_APOLLO_JOIN_STATUS_AP,
	ATBM_APOLLO_JOIN_STATUS_SIMPLE_MONITOR,
};

enum atbm_link_status {
	ATBM_APOLLO_LINK_OFF,
	ATBM_APOLLO_LINK_RESERVE,
	ATBM_APOLLO_LINK_SOFT,
	ATBM_APOLLO_LINK_HARD,
	ATBM_APOLLO_LINK_RESET,
	ATBM_APOLLO_LINK_RESET_REMAP,
};

enum atbm_bss_loss_status {
	ATBM_APOLLO_BSS_LOSS_NONE,
	ATBM_APOLLO_BSS_LOSS_CHECKING,
	ATBM_APOLLO_BSS_LOSS_CONFIRMING,
	ATBM_APOLLO_BSS_LOSS_CONFIRMED,
};

struct atbm_link_entry {
	unsigned long			timestamp;
	enum atbm_link_status		status;
	enum atbm_link_status		prev_status;
	u8				mac[ETH_ALEN];
	u8				buffered[ATBM_APOLLO_MAX_TID];
	struct sk_buff_head		rx_queue;
	u8 mfp;
};
#ifdef CONFIG_ATBM_APOLLO_DEBUG
struct atbm_debug_common {
#ifdef CONFIG_ATBM_APOLLO_DEBUGFS
	struct dentry *debugfs_phy;
#endif
	int tx_cache_miss;
	int tx_burst;
	int rx_burst;
	int ba_cnt;
	int ba_acc;
	int ba_cnt_rx;
	int ba_acc_rx;
};

struct atbm_debug_priv {
#ifdef CONFIG_ATBM_APOLLO_DEBUGFS
	struct dentry *debugfs_phy;
#endif
	int tx;
	int tx_agg;
	int rx;
	int rx_agg;
	int tx_multi;
	int tx_multi_frames;
	int tx_align;
	int tx_ttl;
};
#endif
#ifdef CONFIG_ATBM_APOLLO_TESTMODE
struct advance_scan_elems {
	u8 scanMode;
	u16 duration;
};
/**
 * atbm_tsm_info - Keeps information about ongoing TSM collection
 * @ac: Access category for which metrics to be collected
 * @use_rx_roaming: Use received voice packets to compute roam delay
 * @sta_associated: Set to 1 after association
 * @sta_roamed: Set to 1 after successful roaming
 * @roam_delay: Roam delay
 * @rx_timestamp_vo: Timestamp of received voice packet
 * @txconf_timestamp_vo: Timestamp of received tx confirmation for
 * successfully transmitted VO packet
 * @sum_pkt_q_delay: Sum of packet queue delay
 * @sum_media_delay: Sum of media delay
 *
 */
struct atbm_tsm_info {
	u8 ac;
	u8 use_rx_roaming;
	u8 sta_associated;
	u8 sta_roamed;
	u16 roam_delay;
	u32 rx_timestamp_vo;
	u32 txconf_timestamp_vo;
	u32 sum_pkt_q_delay;
	u32 sum_media_delay;
};

/**
 * atbm_start_stop_tsm - To start or stop collecting TSM metrics in
 * apollo driver
 * @start: To start or stop collecting TSM metrics
 * @up: up for which metrics to be collected
 * @packetization_delay: Packetization delay for this TID
 *
 */
struct atbm_start_stop_tsm {
	u8 start;       /*1: To start, 0: To stop*/
	u8 up;
	u16 packetization_delay;
};

#endif /* CONFIG_ATBM_APOLLO_TESTMODE */
#define MAX_SCAN_INFO_NUM 40
struct atbmwifi_scan_result_info{
	u8 ssid[33];
	u8 ssidlen;
	u8 channel;
	u8 ht:1,
	   wpa:1,
	   rsn:1,
	   wps:1,
	   p2p:1,
	   b40M:1,
	   encrypt:1;
};

struct atbmwifi_scan_result{
	int len;
	struct atbmwifi_scan_result_info *info;
};
struct hmac_configure {
	u8 ssid[32];
	u8 ssid_len;
	u8 bssid[ETH_ALEN];
	u8 password_len;
	u8 password[64];
	u8 privacy:1,
		psk_set:1,
		bssid_set:1,
		fixrate_set:1;
	u8 ap_mode;
	u8 hw_fix_rate_id;
	u8 reserve;

	u8 key_mgmt;
	u8 key_id;
	u16 auth_alg;
};

struct atbm_he_info {
#ifdef CONFIG_ATBM_HE	
	struct atbm_ieee80211_sta_he_cap he_cap;
#endif //CONFIG_ATBM_HE
};

struct hif_tracing_index
{
	struct list_head head;
	int				 start_idx;
	int              end_idx;
	u32              start_addr;
	u32              end_addr;
};

enum wsm_hif_trace_result
{
	HIF_TRACE_RESULT_IGNORE,
	HIF_TRACE_RESULT_COMP,
	HIF_TRACE_RESULT_FAIL,
};
struct wsm_hif_append
{
	u32 sm_addr;
	u32 histories;
	u16 len;
	u16 id;
};
struct wsm_hif_trace_info
{
	__le16 len;
	__le16 id;
	__le32 data1;
	__le32 data2;
};
struct hif_trace
{
	int 			wsm_trace;
	u32             wsm_trace_addr;
	int 			wsm_trace_miss;
	int 			wsm_trace_miss_addr;
	atomic_t		bh_tracehif;
	struct sk_buff	*trace_skb;
	struct list_head trace;
	spinlock_t	    lock;
};

struct atbm_common {
	struct atbm_debug_common	*debug;
	struct atbm_queue		tx_queue[4];
	struct atbm_queue_stats	tx_queue_stats;

	struct ieee80211_hw		*hw;
	struct mac_address		addresses[ATBM_WIFI_MAX_VIFS];

	/*Will be a pointer to a list of VIFs - Dynamically allocated */
	struct ieee80211_vif		__rcu *vif_list[ATBM_WIFI_MAX_VIFS+1];
	atomic_t			num_vifs;
	spinlock_t			vif_list_lock;
	u32				if_id_slot;
	struct device			*pdev;
	struct atbm_workqueue_struct		*workqueue;
	struct sk_buff_head			rx_frame_queue;
	struct sk_buff_head			rx_frame_free;
	struct mutex			conf_mutex;

	const struct sbus_ops		*sbus_ops;
	struct sbus_priv		*sbus_priv;
	int init_done;
	/* HW/FW type (HIF_...) */
	int				hw_type;
	int				hw_revision;
	int				fw_revision;

	/* firmware/hardware info */
	unsigned int tx_hdr_len;

	/* Radio data */
	int output_power;
	int noise;
#ifdef ATBM_SUPPORT_SMARTCONFIG
	spinlock_t	spinlock_smart;
	struct atbmwifi_scan_result scan_ret;
	struct smartconfig_config st_cfg;
	int st_status;
	int st_configchannel;
	struct atbm_timer_list smartconfig_expire_timer;
	struct hmac_configure *config;
#endif
	/* calibration, output power limit and rssi<->dBm conversation data */

	/* BBP/MAC state */
#ifdef CONFIG_SUPPORT_SDD
	const struct firmware		*sdd;
#endif
	struct ieee80211_rate		*rates;
	struct ieee80211_rate		*mcs_rates;
    struct ieee80211_rate		*he_mcs_rates;
	u8 mac_addr[ETH_ALEN];
	/*TODO:COMBO: To be made per VIFF after mac80211 support */
	struct ieee80211_channel	*channel;
#ifdef CONFIG_ATBM_STA_LISTEN
	/*sta listen function*/
	struct ieee80211_channel	__rcu *sta_listen_channel;
	int 						sta_listen_if;
	int 						sta_listen_if_save;
#endif
	/*
	*channel_type :record current channel type.
	*maybe changed by action frame.
	*/
	enum nl80211_channel_type channel_type;
	
	atomic_t				  atbm_pluged;
	u8				long_frame_max_tx_count;
	u8				short_frame_max_tx_count;
	/* TODO:COMBO: According to Hong aggregation will happen per VIFF.
	* Keeping in common structure for the time being. Will be moved to VIFF
	* after the mechanism is clear */
	//u8				ba_tid_mask;
	u8 				ba_tid_tx_mask;
	u8				ba_tid_rx_mask;
#ifdef CONFIG_PM
	struct atbm_pm_state		pm_state;
#endif
	bool				is_BT_Present;
	bool				is_go_thru_go_neg;
	u8				conf_listen_interval;

	/* BH */
	atomic_t			bh_rx;
	atomic_t			bh_tx;
	atomic_t			bh_term;
#ifdef SPI_BUS
	atomic_t			hw_ready;
#endif
	atomic_t			bh_suspend;
	atomic_t			bh_suspend_usb;
	atomic_t			bh_halt;
#ifdef ATBM_USB_RESET
	atomic_t			usb_reset;
#endif
#ifdef SDIO_BUS
	bool				hard_irq;
#endif
	struct task_struct		*bh_thread;
	int				bh_error;
	wait_queue_head_t		bh_wq;
	wait_queue_head_t		bh_evt_wq;
	int 			buf_id_offset;
	int				buf_id_tx;	/* byte */
	int				buf_id_rx;	/* byte */
	int				wsm_rx_seq;	/* byte */
	int				wsm_tx_seq;	/* byte */
#ifdef USB_CMD_UES_EP0
	int				wsm_tx_seq_ep0;	/* byte */
#endif
	int				wsm_txframe_num;	/* byte */
	int				wsm_txconfirm_num;	/* byte */
	int				wsm_hifconfirm_num;	/* byte */
	int				wsm_hiftx_num;	/* byte */
	int             buf_id_tx_small;
	int				hw_bufs_used;
	int             hw_bufs_free;
	int				hw_bufs_free_init;
	u32 			n_xmits;
	u32				hw_xmits;
	int 			hw_noconfirm_tx;
	int				hw_bufs_used_vif[ATBM_WIFI_MAX_VIFS];
	struct sk_buff			*skb_cache;
	bool				device_can_sleep;
	/* Keep apollo awake (WUP = 1) 1 second after each scan to avoid
	 * FW issue with sleeping/waking up. */
	atomic_t			recent_scan;

	/* WSM */
	struct wsm_caps			wsm_caps;
	struct mutex			wsm_cmd_mux;
	struct wsm_buf			wsm_cmd_buf;
	struct wsm_cmd			wsm_cmd;
	wait_queue_head_t		wsm_cmd_wq;
	wait_queue_head_t		wsm_startup_done;
	struct wsm_cbc			wsm_cbc;
	atomic_t			tx_lock;
	u32				pending_frame_id;
#ifdef CONFIG_ATBM_APOLLO_TESTMODE
	/* Device Power Range */
	struct wsm_tx_power_range       txPowerRange[2];
	/* Advance Scan */
	struct advance_scan_elems	advanceScanElems;
	bool				enable_advance_scan;
	struct atbm_delayed_work		advance_scan_timeout;
#endif /* CONFIG_ATBM_APOLLO_TESTMODE */

	/* WSM debug */
	int				wsm_enable_wsm_dumps;
	u32				wsm_dump_max_size;

	/* Scan status */
	struct atbm_scan scan;

	/* TX/RX */
	unsigned long		rx_timestamp;
	unsigned long       tx_timestamp;
	/* Scan Timestamp */
	unsigned long		scan_timestamp; 
	spinlock_t		tx_com_lock;
	/* cryptographic engine information */

	/* bit field of glowing LEDs */
	u16 softled_state;

	/* statistics */
	struct ieee80211_low_level_stats stats;

	struct atbm_ht_info		ht_info;
#ifdef CONFIG_ATBM_HE
	struct atbm_he_info		he_info;
#endif  //#ifdef CONFIG_ATBM_HE
	int				tx_burst_idx;

	struct ieee80211_iface_limit		if_limits1[2];
	struct ieee80211_iface_limit		if_limits2[2];
	struct ieee80211_iface_limit		if_limits3[2];
	struct ieee80211_iface_combination	if_combs[3];
	struct semaphore		wsm_oper_lock;
	struct atbm_timer_list		wsm_pm_timer;
	spinlock_t				wsm_pm_spin_lock;
	atomic_t				wsm_pm_running;
#ifdef CONFIG_ATBM_SUPPORT_P2P
	struct atbm_delayed_work		rem_chan_timeout;
	atomic_t			remain_on_channel;
	unsigned long roc_start_time;
	unsigned long roc_duration;
	int				roc_if_id;
	int				roc_not_send;
	u64				roc_cookie;
#endif
	int 			monitor_if_id;
	u16				prev_channel;
	int       if_id_selected;
	u32				key_map;
	struct wsm_add_key		keys[WSM_KEY_MAX_INDEX + 1];
#ifdef MCAST_FWDING
	struct wsm_buf		wsm_release_buf[WSM_MAX_BUF];
	u8			buf_released;
#endif
#ifdef CONFIG_ATBM_SUPPORT_SCHED_SCAN
#ifdef ROAM_OFFLOAD
	u8				auto_scanning;
	u8				frame_rcvd;
	u8				num_scanchannels;
	u8				num_2g_channels;
	u8				num_5g_channels;
	struct wsm_scan_ch		scan_channels[WSM_SCAN_MAX_NUM_OF_CHANNELS];
	struct sk_buff 			*beacon;
	struct sk_buff 			*beacon_bkp;
#endif /*ROAM_OFFLOAD*/
#endif
#ifdef CONFIG_ATBM_APOLLO_TESTMODE
	struct atbm_tsm_stats		tsm_stats;	
	struct atbm_tsm_stats		atbm_tsm_stats[4];
	struct atbm_tsm_info		tsm_info;
	spinlock_t			tsm_lock;
	struct atbm_start_stop_tsm	start_stop_tsm;
#endif /* CONFIG_ATBM_APOLLO_TESTMODE */
	u8          connected_sta_cnt;
#ifdef SPI_BUS
	struct atbm_timer_list spi_status_rx_ready_timer;
#endif
	//WAKELOCK for android
	spinlock_t wakelock_spinlock;
	u32 wakelock_hw_counter;
	u32 wakelock_bh_counter;
#if defined(CONFIG_HAS_WAKELOCK) && (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 27))
	struct wake_lock hw_wake;	/* Wifi hw wakelock */
	struct wake_lock bh_wake;   /* Wifi bh_wake wakelock */
#endif

//ETF
	u32 etf_channel;
	u32 etf_channel_type;
	u32 etf_rate;
	u32 etf_len;	
	u32 etf_greedfiled;
	u8 bStartTx;
	u8 bStartTxWantCancel;	
	u8 etf_test_v2;
	//struct atbm_timer_list etf_expire_timer;
	struct atbm_work_struct etf_tx_end_work;
#ifdef CONFIG_IEEE80211_SEND_SPECIAL_MGMT	
	struct atbm_work_struct send_prvmgmt_work;
	struct sk_buff_head send_prvmgmt_skb_queue;
	struct task_struct		*send_prvmgmt_thread;
	wait_queue_head_t		send_prvmgmt_wq;
	bool stop_prvmgmt_thread;
	bool start_send_prbresp;
	bool start_send_action;
//	struct mutex stop_send_prbresp_lock;	
//	struct atbm_vendor_cfg_ie private_ie;
	struct atbm_ap_vendor_cfg_ie ap_vendor_cfg_ie;
	struct atbm_customer_action customer_action_ie;  
#endif		


	struct efuse_headr efuse;
	u32 chip_version;
	bool loader_ble;

/*
*	ieee80211 rx tasklet schedule
*/
#ifdef ATBM_P2P_CHANGE
	/*
	*only use when if_id = 1
	*/
		u8 go_bssid[6];
		atomic_t go_bssid_set;
		atomic_t p2p_oper_channel;
		atomic_t receive_go_resp;
		atomic_t combination;
		atomic_t operating_channel_combination;
		unsigned long p2p_scan_start_time;
#endif

	u8 *xmit_buff;
	struct sk_buff	*recv_skb;
	bool bh_running;
	u8 multrx_trace[64];
	u8 multrx_index;
#ifdef  CONFIG_SUPPORT_HIF_TRACE
	struct hif_trace trace;
#endif

};

/* Virtual Interface State. One copy per VIF */
struct atbm_vif {
	atomic_t			enabled;
	spinlock_t			vif_lock;
	int				if_id;
	/*TODO: Split into Common and VIF parts */
#ifdef CONFIG_ATBM_APOLLO_DEBUG
	struct atbm_debug_priv	debug;
#endif
	/* BBP/MAC state */
	u8 bssid[ETH_ALEN];
	struct wsm_edca_params		edca;
	struct wsm_tx_queue_params	tx_queue_params;
	struct wsm_association_mode	association_mode;
	struct wsm_set_bss_params	bss_params;
	struct wsm_set_pm		powersave_mode;
	struct wsm_set_pm		firmware_ps_mode;
	struct wsm_set_pm       saved_ps_mode;
	int				user_power_set_true;
	u8				user_pm_mode;
	bool			tmpframe_probereq_set;
	int				cqm_rssi_thold;
	unsigned			cqm_rssi_hyst;
	unsigned			cqm_tx_failure_thold;
	unsigned			cqm_tx_failure_count;
	bool				cqm_use_rssi;
	int				cqm_link_loss_count;
	int				cqm_beacon_loss_count;
	int				mode;
	bool				enable_beacon;
	int				beacon_int;
	int				ssid_length;
	u8				ssid[IEEE80211_MAX_SSID_LEN];
#ifdef HIDDEN_SSID
	bool				hidden_ssid;
#endif
	bool				listening;
	struct wsm_rx_filter		rx_filter;
	struct wsm_beacon_filter_table	bf_table;
	struct wsm_beacon_filter_control bf_control;
	struct wsm_multicast_filter	multicast_filter;
	bool				has_multicast_subscription;
	struct wsm_broadcast_addr_filter	broadcast_filter;
	bool				disable_beacon_filter;
	struct wsm_arp_ipv4_filter      filter4;
#ifdef IPV6_FILTERING
	struct wsm_ndp_ipv6_filter 	filter6;
#endif /*IPV6_FILTERING*/
#ifdef CONFIG_PM
	struct atbm_pm_state_vif	pm_state_vif;
#endif
	/*TODO: Add support in mac80211 for psmode info per VIF */
	struct wsm_p2p_ps_modeinfo	p2p_ps_modeinfo;
	struct wsm_uapsd_info		uapsd_info;
	bool				setbssparams_done;
	u32				listen_interval;
	u32				erp_info;
	bool				powersave_enabled;

	/* WSM Join */
	enum atbm_join_status	join_status;
	u8			join_bssid[ETH_ALEN];
	
	/* ROC implementation */
	int			join_dtim_period;

	/* Security */
	s8			wep_default_key_id;
    unsigned long           rx_timestamp;
    u32                     cipherType;


	/* AP powersave */
	u32			link_id_map;
	struct atbm_link_entry link_id_db[ATBMWIFI_MAX_STA_IN_AP_MODE];
	u32			sta_asleep_mask;
	u32			pspoll_mask;
	bool			aid0_bit_set;
	spinlock_t		ps_state_lock;
	bool			buffered_multicasts;
	bool			tx_multicast;
	struct atbm_work_struct	set_tim_work;
	struct atbm_work_struct	multicast_start_work;
	struct atbm_work_struct	multicast_stop_work;
	struct atbm_timer_list	mcast_timeout;
	struct ieee80211_vif	*vif;
	struct atbm_common	*hw_priv;
	struct ieee80211_hw	*hw;
	bool			htcap;
#ifdef ATBM_SUPPORT_SMARTCONFIG
	u8 scan_expire;
	u8 scan_no_connect_back;
	u8 scan_no_connect;
	u8 auto_connect_when_lost;
#endif
#ifdef	ATBM_WIFI_QUEUE_LOCK_BUG
	u16 queue_cap;
#endif
	struct ieee80211_sta __rcu * linked_sta[WLAN_LINK_ID_MAX+1+1];

};
struct atbm_sta_priv {
	int link_id;
	struct atbm_vif *priv;
};
enum atbm_data_filterid {
	IPV4ADDR_FILTER_ID = 0,
#ifdef IPV6_FILTERING
	IPV6ADDR_FILTER_ID,
#endif /*IPV6_FILTERING*/
};
struct hif_tx_encap{
	u8 *data;
	u32 tx_len; 
	int burst; 
	u8 *source;
};
struct wsm_tx_encap{
	struct atbm_vif *priv;
	struct atbm_queue *queue;
	struct sk_buff* skb;
	struct atbm_txpriv *txpriv;
	u8 **source;
	u8 *data;
	u32 tx_allowed_mask;
	u32 packetID;
	size_t len;
	bool more;
};

struct wsm_rx_encap{
	struct atbm_common *hw_priv;
	struct sk_buff* skb;
	bool (*rx_func)(struct wsm_rx_encap *encap,struct sk_buff* skb);
	bool check_seq;
	bool gro_flush;
	bool suspend;
	struct wsm_hdr *wsm;
	int wsm_id;
	void *priv;
#ifdef  CONFIG_SUPPORT_HIF_TRACE
	void *trace;
#endif
};
static inline
struct atbm_common *ABwifi_vifpriv_to_hwpriv(struct atbm_vif *priv)
{
	return priv->hw_priv;
}


static inline
struct atbm_vif *ABwifi_get_vif_from_ieee80211(struct ieee80211_vif *vif)
{
	return  vif ? (struct atbm_vif *)vif->drv_priv:NULL;
}
static inline void atbm_hw_vif_read_lock(spinlock_t *lock)
{
	rcu_read_lock();
}
static inline void atbm_hw_vif_read_unlock(spinlock_t *lock)
{
	rcu_read_unlock();
}

static inline void atbm_hw_vif_write_lock(spinlock_t *lock)
{
	spin_lock_bh(lock);
}
static inline void atbm_hw_vif_write_unlock(spinlock_t *lock)
{
	spin_unlock_bh(lock);
}

static inline void atbm_priv_vif_list_read_lock(spinlock_t *lock)
{
	rcu_read_lock();
}

static inline void atbm_priv_vif_list_read_unlock(spinlock_t *lock)
{
	rcu_read_unlock();
}

static inline void atbm_priv_vif_list_write_lock(spinlock_t *lock)
{
	
}

static inline void atbm_priv_vif_list_write_unlock(spinlock_t *lock)
{

}

#define ATBM_HW_VIF_SET(vif_list,vif)			rcu_assign_pointer(vif_list,vif)	
#define ATBM_HW_VIF_GET(vif_list)				rcu_dereference(vif_list)


static inline
struct atbm_vif *ABwifi_hwpriv_to_vifpriv(struct atbm_common *hw_priv,
						int if_id)
{
	struct atbm_vif *priv;
	struct ieee80211_vif *vif;
	
	atbm_hw_vif_read_lock(&hw_priv->vif_list_lock);
	if (((-1 == if_id) || (if_id > ATBM_WIFI_MAX_VIFS))){
		atbm_printk_err("if_id = %d\n", if_id);
		atbm_hw_vif_read_unlock(&hw_priv->vif_list_lock);
		return NULL;
	}

	vif = ATBM_HW_VIF_GET(hw_priv->vif_list[if_id]);
	if(!vif){
		atbm_hw_vif_read_unlock(&hw_priv->vif_list_lock);
		return NULL;
	}

	priv = ABwifi_get_vif_from_ieee80211(vif);
	WARN_ON(!priv);
	if(!priv)
		atbm_hw_vif_read_unlock(&hw_priv->vif_list_lock);
	return priv;
}

static inline
struct atbm_vif *__ABwifi_hwpriv_to_vifpriv(struct atbm_common *hw_priv,
					      int if_id)
{
	if(WARN_ON((-1 == if_id) || (if_id > ATBM_WIFI_MAX_VIFS))){
		return NULL;
	}
	/* TODO:COMBO: During scanning frames can be received
	 * on interface ID 3 */
	if (!hw_priv->vif_list[if_id]) {
		return NULL;
	}

	return ABwifi_get_vif_from_ieee80211(hw_priv->vif_list[if_id]);
}

static inline
struct atbm_vif *ABwifi_get_activevif(struct atbm_common *hw_priv)
{
	return ABwifi_hwpriv_to_vifpriv(hw_priv, ffs(hw_priv->if_id_slot)-1);
}



static inline bool is_hardware_APOLLO(struct atbm_common *hw_priv)
{
	return (hw_priv->hw_revision == ATBM_APOLLO_REV_1601);
}

static inline int ABwifi_get_nr_hw_ifaces(struct atbm_common *hw_priv)
{
	return 1;
}

#ifdef IPV6_FILTERING
/* IPV6 host addr info */
struct ipv6_addr_info {
	u8 filter_mode;
	u8 address_mode;
	u16 ipv6[8];
};
#endif /*IPV6_FILTERING*/
#ifndef USB_BUS
extern int atbm_wtd_term(struct atbm_common *hw_priv);
#endif

/* interfaces for the drivers */
int atbm_core_probe(const struct sbus_ops *sbus_ops,
		      struct sbus_priv *sbus,
		      struct device *pdev,
		      struct atbm_common **pself);
void atbm_core_release(struct atbm_common *self);
#ifdef ATBM_WIFI_QUEUE_LOCK_BUG
static inline void atbm_stop_active_vif_queues(struct ieee80211_vif *vif)
{
	struct atbm_vif *priv = ABwifi_get_vif_from_ieee80211(vif);
	int i = 0;
	struct atbm_common *hw_priv = priv->hw_priv;
	BUG_ON(hw_priv == NULL);
	for (i = 0; i < 4; ++i){
		atbm_queue_lock(&hw_priv->tx_queue[i],priv->if_id);
	}
} 

static inline void atbm_wake_active_vif_queues(struct ieee80211_vif *vif)
{
	struct atbm_vif *priv = ABwifi_get_vif_from_ieee80211(vif);
	int i = 0;
	struct atbm_common *hw_priv = priv->hw_priv;
	BUG_ON(hw_priv == NULL);
	for (i = 0; i < 4; ++i)
		atbm_queue_unlock(&hw_priv->tx_queue[i],priv->if_id);
}

static inline void atbm_process_active_vif_queues(void *data, u8 *mac,struct ieee80211_vif *vif)
{
	int lock = (*(int *)data);

	if(lock)
		atbm_stop_active_vif_queues(vif);
	else
		atbm_wake_active_vif_queues(vif);
}
#endif
static inline void atbm_tx_queues_lock(struct atbm_common *hw_priv)
{
	#ifndef ATBM_WIFI_QUEUE_LOCK_BUG
	int i;
	for (i = 0; i < 4; ++i)
		atbm_queue_lock(&hw_priv->tx_queue[i]);
	#else
	#if 0
	int i = 1;
	ieee80211_iterate_active_interfaces_atomic(hw_priv->hw,atbm_process_active_vif_queues,(void*)(&i));
	#else
	int if_id = 0;
	int i = 0;

	for(if_id = 0;if_id<ATBM_WIFI_MAX_VIFS;if_id++){
		for (i = 0; i < 4; ++i){
			atbm_queue_lock(&hw_priv->tx_queue[i],if_id);
		}
	}
	#endif
	#endif
}

static inline void atbm_tx_queues_unlock(struct atbm_common *hw_priv)
{
	#ifndef ATBM_WIFI_QUEUE_LOCK_BUG
	int i;
	for (i = 0; i < 4; ++i)
		atbm_queue_unlock(&hw_priv->tx_queue[i]);
	#else
	#if 0
	int i = 0;
	ieee80211_iterate_active_interfaces_atomic(hw_priv->hw,atbm_process_active_vif_queues,(void*)(&i));
	#else
	int if_id = 0;
	int i = 0;
	for(if_id = 0;if_id<ATBM_WIFI_MAX_VIFS;if_id++){
		for (i = 0; i < 4; ++i){
			atbm_queue_unlock(&hw_priv->tx_queue[i],if_id);
		}
	}
	#endif
	#endif
}

/* Datastructure for LLC-SNAP HDR */
#define P80211_OUI_LEN  3

struct ieee80211_snap_hdr {
        u8    dsap;   /* always 0xAA */
        u8    ssap;   /* always 0xAA */
        u8    ctrl;   /* always 0x03 */
        u8    oui[P80211_OUI_LEN];    /* organizational universal id */
} __packed;



#define atbm_for_each_vif(_hw_priv, _priv, _i)			\
for (									\
	_i = 0;								\
	_priv = _hw_priv->vif_list[_i]? 				\
	ABwifi_get_vif_from_ieee80211(_hw_priv->vif_list[_i]) : NULL,	\
	_i < ATBM_WIFI_MAX_VIFS;						\
	_i++								\
)

#define atbm_for_each_vif_safe(_hw_priv, _priv, _i)			\
for (									\
	_i = 0;								\
	_priv = ABwifi_get_vif_from_ieee80211(ATBM_HW_VIF_GET(_hw_priv->vif_list[_i])),	\
	_i < ATBM_WIFI_MAX_VIFS;						\
	_i++								\
)
#define ATBM_VOLATILE_SET(s,v)	*((volatile typeof(*(s)) *)(s)) = (v);smp_mb()
#define ATBM_VOLATILE_GET(s)	(*((volatile typeof(*(s)) *)(s)))

#define atbm_hw_priv_assign_pointer(p)	ATBM_VOLATILE_SET(&g_hw_priv,p)
#define atbm_hw_priv_dereference()		ATBM_VOLATILE_GET(&g_hw_priv)
#include  "debug.h"
#ifdef CONFIG_ATBM_APOLLO_USE_GPIO_IRQ
void atbm_oob_intr_set(struct sbus_priv *self, bool enable);
#endif

#ifdef	ATBM_WIFI_QUEUE_LOCK_BUG
extern void atbm_set_priv_queue_cap(struct atbm_vif *priv);
extern void atbm_clear_priv_queue_cap(struct atbm_vif *priv);
#endif
extern int atbm_save_efuse(struct atbm_common *hw_priv,struct efuse_headr *efuse_save);
extern void atbm_destroy_wsm_cmd(struct atbm_common *hw_priv);
extern int atbm_reinit_firmware(struct atbm_common *hw_priv);
extern int atbm_rx_bh_flush(struct atbm_common *hw_priv);
extern void atbm_bh_halt(struct atbm_common *hw_priv);
extern void atbm_hif_status_set(int status);
extern void  atbm_bh_multrx_trace(struct atbm_common *hw_priv,u8 n_rx);
void atbm_get_drv_version(short *p);
extern bool atbm_support_tid_aggr(struct atbm_common *hw_priv,u8 tid);
void atbm_build_rate_policy(struct wsm_tx_encap *wsm_encap);
void atbm_build_wsm_hdr(struct wsm_tx_encap *wsm_encap);
bool atbm_rx_serial(struct wsm_rx_encap *encap);
void atbm_rx_multi_rx(struct wsm_rx_encap *encap);
struct sk_buff * atbm_rx_alloc_frame(struct wsm_rx_encap *encap);
bool atbm_rx_tasklet_process_encap(struct wsm_rx_encap *encap,struct sk_buff *skb);
extern bool atbm_rx_directly(struct wsm_rx_encap *encap);
bool atbm_process_wsm_cmd_frame(struct wsm_rx_encap *encap);
#ifdef CONFIG_ATBM_SUPPORT_CSA
int atbm_do_csa_concurrent(struct ieee80211_hw *hw,struct list_head *head);
#endif
#ifdef  CONFIG_SUPPORT_HIF_TRACE
bool  atbm_hif_trace_running(struct atbm_common *hw_priv);
bool  atbm_hif_trace_process(struct atbm_common *hw_priv);
enum wsm_hif_trace_result atbm_hif_trace_check(struct wsm_hdr *wsm,size_t true_len);
bool atbm_hif_trace_process_miss(struct atbm_common *hw_priv,struct wsm_hif_trace_info *info);
bool atbm_hif_trace_init(struct atbm_common *hw_priv);
bool atbm_hif_trace_exit(struct atbm_common *hw_priv);
void atbm_hif_trace_restart(struct atbm_common *hw_priv);
#else
#define atbm_hif_trace_prepare(_encap)
#define atbm_hif_trace_post(_encap)
#define atbm_hif_trace_running(_hw_priv) false
#define atbm_hif_trace_process(_hw_priv) false
#define atbm_hif_trace_check(_wsm,len)	 HIF_TRACE_RESULT_IGNORE
#define atbm_hif_trace_process_miss(_hw_priv,_info) false
static inline bool atbm_hif_trace_init(struct atbm_common *hw_priv) {return true;}
static inline bool atbm_hif_trace_exit(struct atbm_common *hw_priv) {return true;}
static inline void atbm_hif_trace_restart(struct atbm_common *hw_priv) {};
#endif

#define CAPABILITIES_ATBM_PRIVATE_IE 		BIT(1)
#define CAPABILITIES_NVR_IPC 				BIT(2)
#define CAPABILITIES_NO_CONFIRM 			BIT(3)
#define CAPABILITIES_WOW 					BIT(4)
#define CAPABILITIES_NO_BACKOFF 			BIT(5)
#define CAPABILITIES_CFO 					BIT(6)
//#define CAPABILITIES_AGC 					BIT(7)
#define CAPABILITIES_CHIP_FLAG 				BIT(7)
#define CAPABILITIES_TXCAL 					BIT(8)
#define CAPABILITIES_HIF_TRACE 				BIT(9)
#define CAPABILITIES_FORCE_PM_ACTIVE 		BIT(10)
#define CAPABILITIES_SMARTCONFIG			BIT(11)
#define CAPABILITIES_ETF					BIT(12)
//#define CAPABILITIES_LMAC_RATECTL			BIT(13)
//#define CAPABILITIES_LMAC_TPC				BIT(14)
//#define CAPABILITIES_LMAC_TEMPC				BIT(15)
#define CAPABILITIES_BLE					BIT(13)
#define CAPABILITIES_40M					BIT(14)
#define CAPABILITIES_LDPC					BIT(15)
#define CAPABILITIES_NAV_ENABLE				BIT(16)
#define CAPABILITIES_USB_RECOVERY_BUG		BIT(17)
#define CAPABILITIES_VIFADDR_LOCAL_BIT		BIT(18)
#define CAPABILITIES_USE_IPC 				BIT(19)
#define CAPABILITIES_OUTER_PA 				BIT(20)
#define CAPABILITIES_WFA 					BIT(21)
#define CAPABILITIES_EAP_GROUP_REKEY 		BIT(22)
#define CAPABILITIES_RTS_LONG_DURATION 		BIT(23)
#define CAPABILITIES_TX_CFO_PPM_CORRECTION 	BIT(24)
#define CAPABILITIES_RXCAL 					BIT(25)
#define CAPABILITIES_HW_CHECKSUM  			BIT(26)
#define CAPABILITIES_80211_TO_8023 			BIT(27)
#define CAPABILITIES_NEW_CHIPER 			BIT(28)
#define CAPABILITIES_EFUSE8 BIT(29)
#define CAPABILITIES_EFUSEI BIT(30)
#define CAPABILITIES_EFUSEB BIT(31)

#define EX_CAPABILITIES_TWO_CHIP_ONE_SOC			BIT(0)
#define EX_CAPABILITIES_MANUAL_SET_AC				BIT(1)
#define EX_CAPABILITIES_LMAC_BW_CONTROL				BIT(2)
#define EX_CAPABILITIES_SUPPORT_TWO_ANTENNA			BIT(3)
#define EX_CAPABILITIES_ENABLE_STA_REMAIN_ON_CHANNEL BIT(4)
#define EX_CAPABILITIES_ENABLE_PS					BIT(5)
#define EX_CAPABILITIES_TX_REQUEST_FIFO_LINK		BIT(6)
#define EX_CAPABILITIES_CHIP_TYPE					(BIT(7)|BIT(8)|BIT(9))

typedef struct{
	char *name;
	int wsm_cap_flag;
}atbm_caps_display;

static inline int atbm_hif_trace_driver_cap(struct atbm_common *hw_priv)
{
	BUG_ON(hw_priv == NULL);
#ifdef  CONFIG_SUPPORT_HIF_TRACE
	return 1;
#else
	return 0;
#endif
}
static inline int atbm_hif_trace_firmware_cap(struct atbm_common *hw_priv)
{
	return !!(hw_priv->wsm_caps.firmwareCap & CAPABILITIES_HIF_TRACE);
}

static inline int atbm_hif_trace_cap_valid(struct atbm_common *hw_priv)
{
	return atbm_hif_trace_driver_cap(hw_priv) ^ atbm_hif_trace_firmware_cap(hw_priv);
}
#ifdef CONFIG_ATBM_BLE
int atbm_ble_do_ble_xmit(struct ieee80211_hw *hw,u8* xmit,size_t len);
#endif
#endif /* ATBM_APOLLO_H */
