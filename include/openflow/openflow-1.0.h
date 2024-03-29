/*
 * Copyright (c) 2008, 2009, 2010, 2011, 2012 Nicira, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* OpenFlow: protocol between controller and datapath. */

#ifndef OPENFLOW_OPENFLOW10_H
#define OPENFLOW_OPENFLOW10_H 1

#include "openflow/openflow-common.h"

/* Port numbering.  Physical ports are numbered starting from 1. */
enum ofp_port {
    /* Maximum number of physical switch ports. */
    OFPP_MAX = 0xff00,

    /* Fake output "ports". */
    OFPP_IN_PORT    = 0xfff8,  /* Send the packet out the input port.  This
                                  virtual port must be explicitly used
                                  in order to send back out of the input
                                  port. */
    OFPP_TABLE      = 0xfff9,  /* Perform actions in flow table.
                                  NB: This can only be the destination
                                  port for packet-out messages. */
    OFPP_NORMAL     = 0xfffa,  /* Process with normal L2/L3 switching. */
    OFPP_FLOOD      = 0xfffb,  /* All physical ports except input port and
                                  those disabled by STP. */
    OFPP_ALL        = 0xfffc,  /* All physical ports except input port. */
    OFPP_CONTROLLER = 0xfffd,  /* Send to controller. */
    OFPP_LOCAL      = 0xfffe,  /* Local openflow "port". */
    OFPP_NONE       = 0xffff   /* Not associated with a physical port. */
};

/* OpenFlow 1.0 specific message types, in addition to the common message
 * types. */
enum ofp10_type {
    /* Controller command messages. */
    OFPT10_PACKET_OUT = 13,     /* Controller/switch message */
    OFPT10_FLOW_MOD,            /* Controller/switch message */
    OFPT10_PORT_MOD,            /* Controller/switch message */

    /* Statistics messages. */
    OFPT10_STATS_REQUEST,       /* Controller/switch message */
    OFPT10_STATS_REPLY,         /* Controller/switch message */

    /* Barrier messages. */
    OFPT10_BARRIER_REQUEST,     /* Controller/switch message */
    OFPT10_BARRIER_REPLY,       /* Controller/switch message */

    /* Queue Configuration messages. */
    OFPT10_QUEUE_GET_CONFIG_REQUEST,  /* Controller/switch message */
    OFPT10_QUEUE_GET_CONFIG_REPLY     /* Controller/switch message */
};

/* OFPT_HELLO.  This message has an empty body, but implementations must
 * ignore any data included in the body, to allow for future extensions. */
struct ofp_hello {
    struct ofp_header header;
};

#define OFP_DEFAULT_MISS_SEND_LEN   128

enum ofp_config_flags {
    /* Handling of IP fragments. */
    OFPC_FRAG_NORMAL   = 0,  /* No special handling for fragments. */
    OFPC_FRAG_DROP     = 1,  /* Drop fragments. */
    OFPC_FRAG_REASM    = 2,  /* Reassemble (only if OFPC_IP_REASM set). */
    OFPC_FRAG_NX_MATCH = 3,  /* Make first fragments available for matching. */
    OFPC_FRAG_MASK     = 3,

    /* TTL processing - applicable for IP and MPLS packets. */
    OFPC_INVALID_TTL_TO_CONTROLLER = 1 << 2, /* Send packets with invalid TTL
                                                to the controller. */
};

/* Switch configuration. */
struct ofp_switch_config {
    struct ofp_header header;
    ovs_be16 flags;             /* OFPC_* flags. */
    ovs_be16 miss_send_len;     /* Max bytes of new flow that datapath should
                                   send to the controller. */
};
OFP_ASSERT(sizeof(struct ofp_switch_config) == 12);

/* OpenFlow 1.0 specific capabilities supported by the datapath (struct
 * ofp_switch_features, member capabilities). */
enum ofp10_capabilities {
    OFPC10_STP            = 1 << 3,  /* 802.1d spanning tree. */
    OFPC10_RESERVED       = 1 << 4,  /* Reserved, must not be set. */
};

/* OpenFlow 1.0 specific flags to indicate behavior of the physical port.
 * These flags are used in ofp10_phy_port to describe the current
 * configuration.  They are used in the ofp10_port_mod message to configure the
 * port's behavior.
 */
enum ofp10_port_config {
    OFPPC10_NO_STP       = 1 << 1, /* Disable 802.1D spanning tree on port. */
    OFPPC10_NO_RECV_STP  = 1 << 3, /* Drop received 802.1D STP packets. */
    OFPPC10_NO_FLOOD     = 1 << 4, /* Do not include port when flooding. */
#define OFPPC10_ALL (OFPPC_PORT_DOWN | OFPPC10_NO_STP | OFPPC_NO_RECV | \
                     OFPPC10_NO_RECV_STP | OFPPC10_NO_FLOOD | OFPPC_NO_FWD | \
                     OFPPC_NO_PACKET_IN)
};

/* OpenFlow 1.0 specific current state of the physical port.  These are not
 * configurable from the controller.
 */
enum ofp10_port_state {
    /* The OFPPS10_STP_* bits have no effect on switch operation.  The
     * controller must adjust OFPPC_NO_RECV, OFPPC_NO_FWD, and
     * OFPPC_NO_PACKET_IN appropriately to fully implement an 802.1D spanning
     * tree. */
    OFPPS10_STP_LISTEN  = 0 << 8, /* Not learning or relaying frames. */
    OFPPS10_STP_LEARN   = 1 << 8, /* Learning but not relaying frames. */
    OFPPS10_STP_FORWARD = 2 << 8, /* Learning and relaying frames. */
    OFPPS10_STP_BLOCK   = 3 << 8, /* Not part of spanning tree. */
    OFPPS10_STP_MASK    = 3 << 8  /* Bit mask for OFPPS10_STP_* values. */

#define OFPPS10_ALL (OFPPS_LINK_DOWN | OFPPS10_STP_MASK)
};

/* OpenFlow 1.0 specific features of physical ports available in a datapath. */
enum ofp10_port_features {
    OFPPF10_COPPER     = 1 << 7,  /* Copper medium. */
    OFPPF10_FIBER      = 1 << 8,  /* Fiber medium. */
    OFPPF10_AUTONEG    = 1 << 9,  /* Auto-negotiation. */
    OFPPF10_PAUSE      = 1 << 10, /* Pause. */
    OFPPF10_PAUSE_ASYM = 1 << 11  /* Asymmetric pause. */
};

/* Description of a physical port */
struct ofp10_phy_port {
    ovs_be16 port_no;
    uint8_t hw_addr[OFP_ETH_ALEN];
    char name[OFP_MAX_PORT_NAME_LEN]; /* Null-terminated */

    ovs_be32 config;        /* Bitmap of OFPPC_* and OFPPC10_* flags. */
    ovs_be32 state;         /* Bitmap of OFPPS_* and OFPPS10_* flags. */

    /* Bitmaps of OFPPF_* and OFPPF10_* that describe features.  All bits
     * zeroed if unsupported or unavailable. */
    ovs_be32 curr;          /* Current features. */
    ovs_be32 advertised;    /* Features being advertised by the port. */
    ovs_be32 supported;     /* Features supported by the port. */
    ovs_be32 peer;          /* Features advertised by peer. */
};
OFP_ASSERT(sizeof(struct ofp10_phy_port) == 48);

/* Modify behavior of the physical port */
struct ofp10_port_mod {
    struct ofp_header header;
    ovs_be16 port_no;
    uint8_t hw_addr[OFP_ETH_ALEN]; /* The hardware address is not
                                      configurable.  This is used to
                                      sanity-check the request, so it must
                                      be the same as returned in an
                                      ofp10_phy_port struct. */

    ovs_be32 config;        /* Bitmap of OFPPC_* flags. */
    ovs_be32 mask;          /* Bitmap of OFPPC_* flags to be changed. */

    ovs_be32 advertise;     /* Bitmap of "ofp_port_features"s.  Zero all
                               bits to prevent any action taking place. */
    uint8_t pad[4];         /* Pad to 64-bits. */
};
OFP_ASSERT(sizeof(struct ofp10_port_mod) == 32);

/* Query for port queue configuration. */
struct ofp10_queue_get_config_request {
    struct ofp_header header;
    ovs_be16 port;          /* Port to be queried. Should refer
                               to a valid physical port (i.e. < OFPP_MAX) */
    uint8_t pad[2];
    /* 32-bit alignment. */
};
OFP_ASSERT(sizeof(struct ofp10_queue_get_config_request) == 12);

/* Queue configuration for a given port. */
struct ofp10_queue_get_config_reply {
    struct ofp_header header;
    ovs_be16 port;
    uint8_t pad[6];
    /* struct ofp10_packet_queue queues[0]; List of configured queues. */
};
OFP_ASSERT(sizeof(struct ofp10_queue_get_config_reply) == 16);

/* Packet received on port (datapath -> controller). */
struct ofp_packet_in {
    struct ofp_header header;
    ovs_be32 buffer_id;     /* ID assigned by datapath. */
    ovs_be16 total_len;     /* Full length of frame. */
    ovs_be16 in_port;       /* Port on which frame was received. */
    uint8_t reason;         /* Reason packet is being sent (one of OFPR_*) */
    uint8_t pad;
    uint8_t data[0];        /* Ethernet frame, halfway through 32-bit word,
                               so the IP header is 32-bit aligned.  The
                               amount of data is inferred from the length
                               field in the header.  Because of padding,
                               offsetof(struct ofp_packet_in, data) ==
                               sizeof(struct ofp_packet_in) - 2. */
};
OFP_ASSERT(sizeof(struct ofp_packet_in) == 20);

enum ofp10_action_type {
    OFPAT10_OUTPUT,             /* Output to switch port. */
    OFPAT10_SET_VLAN_VID,       /* Set the 802.1q VLAN id. */
    OFPAT10_SET_VLAN_PCP,       /* Set the 802.1q priority. */
    OFPAT10_STRIP_VLAN,         /* Strip the 802.1q header. */
    OFPAT10_PUSH_VLAN,          /* Push the 802.1q VLAN id. */
    OFPAT10_POP_VLAN,           /* Pop the 802.1q VLAN id. */
    OFPAT10_SET_DL_SRC,         /* Ethernet source address. */
    OFPAT10_SET_DL_DST,         /* Ethernet destination address. */
    OFPAT10_SET_ARP_SRC,        /* ARP source address. */
    OFPAT10_SET_ARP_DST,        /* ARP destination address. */
    OFPAT10_SET_NW_SRC,         /* IP source address. */
    OFPAT10_SET_NW_DST,         /* IP destination address. */
    OFPAT10_SET_NW_TOS,         /* IP ToS (DSCP field, 6 bits). */
    OFPAT10_SET_TP_SRC,         /* TCP/UDP source port. */
    OFPAT10_SET_TP_DST,         /* TCP/UDP destination port. */
    OFPAT10_ENQUEUE,            /* Output to queue. */
    OFPAT10_VENDOR = 0xffff
};

/* Action structure for OFPAT10_OUTPUT, which sends packets out 'port'.
 * When the 'port' is the OFPP_CONTROLLER, 'max_len' indicates the max
 * number of bytes to send.  A 'max_len' of zero means no bytes of the
 * packet should be sent. */
struct ofp_action_output {
    ovs_be16 type;                  /* OFPAT10_OUTPUT. */
    ovs_be16 len;                   /* Length is 8. */
    ovs_be16 port;                  /* Output port. */
    ovs_be16 max_len;               /* Max length to send to controller. */
};
OFP_ASSERT(sizeof(struct ofp_action_output) == 8);

/* The VLAN id is 12 bits, so we can use the entire 16 bits to indicate
 * special conditions.  All ones is used to match that no VLAN id was
 * set. */
#define OFP_VLAN_NONE      0xffff

/* Action structure for OFPAT10_SET_VLAN_VID. */
struct ofp_action_vlan_vid {
    ovs_be16 type;                  /* OFPAT10_SET_VLAN_VID. */
    ovs_be16 len;                   /* Length is 8. */
    ovs_be16 vlan_vid;              /* VLAN id. */
    uint8_t pad[2];
};
OFP_ASSERT(sizeof(struct ofp_action_vlan_vid) == 8);

/* Action structure for OFPAT10_SET_VLAN_PCP. */
struct ofp_action_vlan_pcp {
    ovs_be16 type;                  /* OFPAT10_SET_VLAN_PCP. */
    ovs_be16 len;                   /* Length is 8. */
    uint8_t vlan_pcp;               /* VLAN priority. */
    uint8_t pad[3];
};
OFP_ASSERT(sizeof(struct ofp_action_vlan_pcp) == 8);

/* Action structure for OFPAT10_SET_DL_SRC/DST. */
struct ofp_action_dl_addr {
    ovs_be16 type;                  /* OFPAT10_SET_DL_SRC/DST. */
    ovs_be16 len;                   /* Length is 16. */
    uint8_t dl_addr[OFP_ETH_ALEN];  /* Ethernet address. */
    uint8_t pad[6];
};
OFP_ASSERT(sizeof(struct ofp_action_dl_addr) == 16);

/* Action structure for OFPAT10_SET_ARP_SRC/DST. */
struct ofp_action_arp_addr {
    ovs_be16 type;                  /* OFPAT10_SET_ARP_SRC/DST. */
    ovs_be16 len;                   /* Length is 16. */
    uint8_t arp_addr[OFP_ETH_ALEN]; /* ARP Ethernet address. */
    uint8_t pad[6];
};
OFP_ASSERT(sizeof(struct ofp_action_arp_addr) == 16);

/* Action structure for OFPAT10_SET_NW_SRC/DST. */
struct ofp_action_nw_addr {
    ovs_be16 type;                  /* OFPAT10_SET_TW_SRC/DST. */
    ovs_be16 len;                   /* Length is 8. */
    ovs_be32 nw_addr;               /* IP address. */
};
OFP_ASSERT(sizeof(struct ofp_action_nw_addr) == 8);

/* Action structure for OFPAT10_SET_NW_TOS. */
struct ofp_action_nw_tos {
    ovs_be16 type;                  /* OFPAT10_SET_TW_TOS. */
    ovs_be16 len;                   /* Length is 8. */
    uint8_t nw_tos;                 /* IP TOS (DSCP field, 6 bits). */
    uint8_t pad[3];
};
OFP_ASSERT(sizeof(struct ofp_action_nw_tos) == 8);

/* Action structure for OFPAT10_SET_TP_SRC/DST. */
struct ofp_action_tp_port {
    ovs_be16 type;                  /* OFPAT10_SET_TP_SRC/DST. */
    ovs_be16 len;                   /* Length is 8. */
    ovs_be16 tp_port;               /* TCP/UDP port. */
    uint8_t pad[2];
};
OFP_ASSERT(sizeof(struct ofp_action_tp_port) == 8);

/* Action header for OFPAT10_VENDOR. The rest of the body is vendor-defined. */
struct ofp_action_vendor_header {
    ovs_be16 type;                  /* OFPAT10_VENDOR. */
    ovs_be16 len;                   /* Length is a multiple of 8. */
    ovs_be32 vendor;                /* Vendor ID, which takes the same form
                                       as in "struct ofp_vendor_header". */
};
OFP_ASSERT(sizeof(struct ofp_action_vendor_header) == 8);

/* Action header that is common to all actions.  The length includes the
 * header and any padding used to make the action 64-bit aligned.
 * NB: The length of an action *must* always be a multiple of eight. */
struct ofp_action_header {
    ovs_be16 type;                  /* One of OFPAT10_*. */
    ovs_be16 len;                   /* Length of action, including this
                                       header.  This is the length of action,
                                       including any padding to make it
                                       64-bit aligned. */
    uint8_t pad[4];
};
OFP_ASSERT(sizeof(struct ofp_action_header) == 8);

/* OFPAT10_ENQUEUE action struct: send packets to given queue on port. */
struct ofp_action_enqueue {
    ovs_be16 type;            /* OFPAT10_ENQUEUE. */
    ovs_be16 len;             /* Len is 16. */
    ovs_be16 port;            /* Port that queue belongs. Should
                                 refer to a valid physical port
                                 (i.e. < OFPP_MAX) or OFPP_IN_PORT. */
    uint8_t pad[6];           /* Pad for 64-bit alignment. */
    ovs_be32 queue_id;        /* Where to enqueue the packets. */
};
OFP_ASSERT(sizeof(struct ofp_action_enqueue) == 16);

union ofp_action {
    ovs_be16 type;
    struct ofp_action_header header;
    struct ofp_action_vendor_header vendor;
    struct ofp_action_output output;
    struct ofp_action_vlan_vid vlan_vid;
    struct ofp_action_vlan_pcp vlan_pcp;
    struct ofp_action_nw_addr nw_addr;
    struct ofp_action_nw_tos nw_tos;
    struct ofp_action_tp_port tp_port;
};
OFP_ASSERT(sizeof(union ofp_action) == 8);

/* Send packet (controller -> datapath). */
struct ofp_packet_out {
    struct ofp_header header;
    ovs_be32 buffer_id;           /* ID assigned by datapath or UINT32_MAX. */
    ovs_be16 in_port;             /* Packet's input port (OFPP_NONE if none). */
    ovs_be16 actions_len;         /* Size of action array in bytes. */
    /* Followed by:
     *   - Exactly 'actions_len' bytes (possibly 0 bytes, and always a multiple
     *     of 8) containing actions.
     *   - If 'buffer_id' == UINT32_MAX, packet data to fill out the remainder
     *     of the message length.
     */
};
OFP_ASSERT(sizeof(struct ofp_packet_out) == 16);

enum ofp_flow_mod_command {
    OFPFC_ADD,              /* New flow. */
    OFPFC_MODIFY,           /* Modify all matching flows. */
    OFPFC_MODIFY_STRICT,    /* Modify entry strictly matching wildcards */
    OFPFC_DELETE,           /* Delete all matching flows. */
    OFPFC_DELETE_STRICT     /* Strictly match wildcards and priority. */
};

/* Flow wildcards. */
enum ofp_flow_wildcards {
    OFPFW_IN_PORT    = 1 << 0,  /* Switch input port. */
    OFPFW_DL_VLAN    = 1 << 1,  /* VLAN vid. */
    OFPFW_DL_SRC     = 1 << 2,  /* Ethernet source address. */
    OFPFW_DL_DST     = 1 << 3,  /* Ethernet destination address. */
    OFPFW_DL_TYPE    = 1 << 4,  /* Ethernet frame type. */
    OFPFW_NW_PROTO   = 1 << 5,  /* IP protocol. */
    OFPFW_TP_SRC     = 1 << 6,  /* TCP/UDP source port. */
    OFPFW_TP_DST     = 1 << 7,  /* TCP/UDP destination port. */

    /* IP source address wildcard bit count.  0 is exact match, 1 ignores the
     * LSB, 2 ignores the 2 least-significant bits, ..., 32 and higher wildcard
     * the entire field.  This is the *opposite* of the usual convention where
     * e.g. /24 indicates that 8 bits (not 24 bits) are wildcarded. */
    OFPFW_NW_SRC_SHIFT = 8,
    OFPFW_NW_SRC_BITS = 6,
    OFPFW_NW_SRC_MASK = ((1 << OFPFW_NW_SRC_BITS) - 1) << OFPFW_NW_SRC_SHIFT,
    OFPFW_NW_SRC_ALL = 32 << OFPFW_NW_SRC_SHIFT,

    /* IP destination address wildcard bit count.  Same format as source. */
    OFPFW_NW_DST_SHIFT = 14,
    OFPFW_NW_DST_BITS = 6,
    OFPFW_NW_DST_MASK = ((1 << OFPFW_NW_DST_BITS) - 1) << OFPFW_NW_DST_SHIFT,
    OFPFW_NW_DST_ALL = 32 << OFPFW_NW_DST_SHIFT,

    OFPFW_DL_VLAN_PCP = 1 << 20, /* VLAN priority. */
    OFPFW_NW_TOS = 1 << 21, /* IP ToS (DSCP field, 6 bits). */

    /* Wildcard all fields. */
    OFPFW_ALL = ((1 << 22) - 1)
};

/* The wildcards for ICMP type and code fields use the transport source
 * and destination port fields, respectively. */
#define OFPFW_ICMP_TYPE OFPFW_TP_SRC
#define OFPFW_ICMP_CODE OFPFW_TP_DST

/* Values below this cutoff are 802.3 packets and the two bytes
 * following MAC addresses are used as a frame length.  Otherwise, the
 * two bytes are used as the Ethernet type.
 */
#define OFP_DL_TYPE_ETH2_CUTOFF   0x0600

/* Value of dl_type to indicate that the frame does not include an
 * Ethernet type.
 */
#define OFP_DL_TYPE_NOT_ETH_TYPE  0x05ff

/* The VLAN id is 12-bits, so we can use the entire 16 bits to indicate
 * special conditions.  All ones indicates that no VLAN id was set.
 */
#define OFP_VLAN_NONE      0xffff

/* Fields to match against flows */
struct ofp_match {
    ovs_be32 wildcards;        /* Wildcard fields. */
    ovs_be16 in_port;          /* Input switch port. */
    uint8_t dl_src[OFP_ETH_ALEN]; /* Ethernet source address. */
    uint8_t dl_dst[OFP_ETH_ALEN]; /* Ethernet destination address. */
    ovs_be16 dl_vlan;          /* Input VLAN. */
    uint8_t dl_vlan_pcp;       /* Input VLAN priority. */
    uint8_t pad1[1];           /* Align to 64-bits. */
    ovs_be16 dl_type;          /* Ethernet frame type. */
    uint8_t nw_tos;            /* IP ToS (DSCP field, 6 bits). */
    uint8_t nw_proto;          /* IP protocol or lower 8 bits of
                                  ARP opcode. */
    uint8_t pad2[2];           /* Align to 64-bits. */
    ovs_be32 nw_src;           /* IP source address. */
    ovs_be32 nw_dst;           /* IP destination address. */
    ovs_be16 tp_src;           /* TCP/UDP source port. */
    ovs_be16 tp_dst;           /* TCP/UDP destination port. */
};
OFP_ASSERT(sizeof(struct ofp_match) == 40);

/* Value used in "idle_timeout" and "hard_timeout" to indicate that the entry
 * is permanent. */
#define OFP_FLOW_PERMANENT 0

/* By default, choose a priority in the middle. */
#define OFP_DEFAULT_PRIORITY 0x8000

enum ofp_flow_mod_flags {
    OFPFF_SEND_FLOW_REM = 1 << 0,  /* Send flow removed message when flow
                                    * expires or is deleted. */
    OFPFF_CHECK_OVERLAP = 1 << 1,  /* Check for overlapping entries first. */
    OFPFF_EMERG         = 1 << 2   /* Ramark this is for emergency. */
};

/* Flow setup and teardown (controller -> datapath). */
struct ofp_flow_mod {
    struct ofp_header header;
    struct ofp_match match;      /* Fields to match */
    ovs_be64 cookie;             /* Opaque controller-issued identifier. */

    /* Flow actions. */
    ovs_be16 command;             /* One of OFPFC_*. */
    ovs_be16 idle_timeout;        /* Idle time before discarding (seconds). */
    ovs_be16 hard_timeout;        /* Max time before discarding (seconds). */
    ovs_be16 priority;            /* Priority level of flow entry. */
    ovs_be32 buffer_id;           /* Buffered packet to apply to (or -1).
                                     Not meaningful for OFPFC_DELETE*. */
    ovs_be16 out_port;            /* For OFPFC_DELETE* commands, require
                                     matching entries to include this as an
                                     output port.  A value of OFPP_NONE
                                     indicates no restriction. */
    ovs_be16 flags;               /* One of OFPFF_*. */
    struct ofp_action_header actions[0]; /* The action length is inferred
                                            from the length field in the
                                            header. */
};
OFP_ASSERT(sizeof(struct ofp_flow_mod) == 72);

/* Flow removed (datapath -> controller). */
struct ofp_flow_removed {
    struct ofp_header header;
    struct ofp_match match;   /* Description of fields. */
    ovs_be64 cookie;          /* Opaque controller-issued identifier. */

    ovs_be16 priority;        /* Priority level of flow entry. */
    uint8_t reason;           /* One of OFPRR_*. */
    uint8_t pad[1];           /* Align to 32-bits. */

    ovs_be32 duration_sec;    /* Time flow was alive in seconds. */
    ovs_be32 duration_nsec;   /* Time flow was alive in nanoseconds beyond
                                 duration_sec. */
    ovs_be16 idle_timeout;    /* Idle timeout from original flow mod. */
    uint8_t pad2[2];          /* Align to 64-bits. */
    ovs_be64 packet_count;
    ovs_be64 byte_count;
};
OFP_ASSERT(sizeof(struct ofp_flow_removed) == 88);

/* OFPT_ERROR: Error message (datapath -> controller). */
struct ofp_error_msg {
    struct ofp_header header;

    ovs_be16 type;
    ovs_be16 code;
    uint8_t data[0];          /* Variable-length data.  Interpreted based
                                 on the type and code. */
};
OFP_ASSERT(sizeof(struct ofp_error_msg) == 12);

/* Statistics request or reply message. */
struct ofp_stats_msg {
    struct ofp_header header;
    ovs_be16 type;              /* One of the OFPST_* constants. */
    ovs_be16 flags;             /* Requests: always 0.
                                 * Replies: 0 or OFPSF_REPLY_MORE. */
};
OFP_ASSERT(sizeof(struct ofp_stats_msg) == 12);

enum ofp_stats_reply_flags {
    OFPSF_REPLY_MORE  = 1 << 0  /* More replies to follow. */
};

#define DESC_STR_LEN   256
#define SERIAL_NUM_LEN 32
/* Reply to OFPST_DESC request.  Each entry is a NULL-terminated ASCII
 * string. */
struct ofp_desc_stats {
    struct ofp_stats_msg osm;
    char mfr_desc[DESC_STR_LEN];       /* Manufacturer description. */
    char hw_desc[DESC_STR_LEN];        /* Hardware description. */
    char sw_desc[DESC_STR_LEN];        /* Software description. */
    char serial_num[SERIAL_NUM_LEN];   /* Serial number. */
    char dp_desc[DESC_STR_LEN];        /* Human readable description of
                                          the datapath. */
};
OFP_ASSERT(sizeof(struct ofp_desc_stats) == 1068);

/* Stats request of type OFPST_AGGREGATE or OFPST_FLOW. */
struct ofp_flow_stats_request {
    struct ofp_stats_msg osm;
    struct ofp_match match;   /* Fields to match. */
    uint8_t table_id;         /* ID of table to read (from ofp_table_stats)
                                 or 0xff for all tables. */
    uint8_t pad;              /* Align to 32 bits. */
    ovs_be16 out_port;        /* Require matching entries to include this
                                 as an output port.  A value of OFPP_NONE
                                 indicates no restriction. */
};
OFP_ASSERT(sizeof(struct ofp_flow_stats_request) == 56);

/* Body of reply to OFPST_FLOW request. */
struct ofp_flow_stats {
    ovs_be16 length;          /* Length of this entry. */
    uint8_t table_id;         /* ID of table flow came from. */
    uint8_t pad;
    struct ofp_match match;   /* Description of fields. */
    ovs_be32 duration_sec;    /* Time flow has been alive in seconds. */
    ovs_be32 duration_nsec;   /* Time flow has been alive in nanoseconds
                                 beyond duration_sec. */
    ovs_be16 priority;        /* Priority of the entry. Only meaningful
                                 when this is not an exact-match entry. */
    ovs_be16 idle_timeout;    /* Number of seconds idle before expiration. */
    ovs_be16 hard_timeout;    /* Number of seconds before expiration. */
    uint8_t pad2[6];          /* Align to 64 bits. */
    ovs_32aligned_be64 cookie;       /* Opaque controller-issued identifier. */
    ovs_32aligned_be64 packet_count; /* Number of packets in flow. */
    ovs_32aligned_be64 byte_count;   /* Number of bytes in flow. */
    struct ofp_action_header actions[0]; /* Actions. */
};
OFP_ASSERT(sizeof(struct ofp_flow_stats) == 88);

/* Reply to OFPST_AGGREGATE request. */
struct ofp_aggregate_stats_reply {
    struct ofp_stats_msg osm;
    ovs_32aligned_be64 packet_count; /* Number of packets in flows. */
    ovs_32aligned_be64 byte_count;   /* Number of bytes in flows. */
    ovs_be32 flow_count;      /* Number of flows. */
    uint8_t pad[4];           /* Align to 64 bits. */
};
OFP_ASSERT(sizeof(struct ofp_aggregate_stats_reply) == 36);

/* Body of reply to OFPST_TABLE request. */
struct ofp_table_stats {
    uint8_t table_id;        /* Identifier of table.  Lower numbered tables
                                are consulted first. */
    uint8_t pad[3];          /* Align to 32-bits. */
    char name[OFP_MAX_TABLE_NAME_LEN];
    ovs_be32 wildcards;      /* Bitmap of OFPFW_* wildcards that are
                                supported by the table. */
    ovs_be32 max_entries;    /* Max number of entries supported. */
    ovs_be32 active_count;   /* Number of active entries. */
    ovs_32aligned_be64 lookup_count;  /* # of packets looked up in table. */
    ovs_32aligned_be64 matched_count; /* Number of packets that hit table. */
};
OFP_ASSERT(sizeof(struct ofp_table_stats) == 64);

/* Stats request of type OFPST_PORT. */
struct ofp_port_stats_request {
    struct ofp_stats_msg osm;
    ovs_be16 port_no;        /* OFPST_PORT message may request statistics
                                for a single port (specified with port_no)
                                or for all ports (port_no == OFPP_NONE). */
    uint8_t pad[6];
};
OFP_ASSERT(sizeof(struct ofp_port_stats_request) == 20);

/* Body of reply to OFPST_PORT request. If a counter is unsupported, set
 * the field to all ones. */
struct ofp_port_stats {
    ovs_be16 port_no;
    uint8_t pad[6];          /* Align to 64-bits. */
    ovs_32aligned_be64 rx_packets;     /* Number of received packets. */
    ovs_32aligned_be64 tx_packets;     /* Number of transmitted packets. */
    ovs_32aligned_be64 rx_bytes;       /* Number of received bytes. */
    ovs_32aligned_be64 tx_bytes;       /* Number of transmitted bytes. */
    ovs_32aligned_be64 rx_dropped;     /* Number of packets dropped by RX. */
    ovs_32aligned_be64 tx_dropped;     /* Number of packets dropped by TX. */
    ovs_32aligned_be64 rx_errors; /* Number of receive errors.  This is a
                                     super-set of receive errors and should be
                                     great than or equal to the sum of all
                                     rx_*_err values. */
    ovs_32aligned_be64 tx_errors; /* Number of transmit errors.  This is a
                                     super-set of transmit errors. */
    ovs_32aligned_be64 rx_frame_err; /* Number of frame alignment errors. */
    ovs_32aligned_be64 rx_over_err;  /* Number of packets with RX overrun. */
    ovs_32aligned_be64 rx_crc_err;   /* Number of CRC errors. */
    ovs_32aligned_be64 collisions;   /* Number of collisions. */
};
OFP_ASSERT(sizeof(struct ofp_port_stats) == 104);

/* All ones is used to indicate all queues in a port (for stats retrieval). */
#define OFPQ_ALL      0xffffffff

/* Body for stats request of type OFPST_QUEUE. */
struct ofp_queue_stats_request {
    struct ofp_stats_msg osm;
    ovs_be16 port_no;        /* All ports if OFPP_ALL. */
    uint8_t pad[2];          /* Align to 32-bits. */
    ovs_be32 queue_id;       /* All queues if OFPQ_ALL. */
};
OFP_ASSERT(sizeof(struct ofp_queue_stats_request) == 20);

/* Body for stats reply of type OFPST_QUEUE consists of an array of this
 * structure type. */
struct ofp_queue_stats {
    ovs_be16 port_no;
    uint8_t pad[2];          /* Align to 32-bits. */
    ovs_be32 queue_id;       /* Queue id. */
    ovs_32aligned_be64 tx_bytes;   /* Number of transmitted bytes. */
    ovs_32aligned_be64 tx_packets; /* Number of transmitted packets. */
    ovs_32aligned_be64 tx_errors;  /* # of packets dropped due to overrun. */
};
OFP_ASSERT(sizeof(struct ofp_queue_stats) == 32);

/* Vendor extension stats message. */
struct ofp_vendor_stats_msg {
    struct ofp_stats_msg osm;   /* Type OFPST_VENDOR. */
    ovs_be32 vendor;            /* Vendor ID:
                                 * - MSB 0: low-order bytes are IEEE OUI.
                                 * - MSB != 0: defined by OpenFlow
                                 *   consortium. */
    /* Followed by vendor-defined arbitrary additional data. */
};
OFP_ASSERT(sizeof(struct ofp_vendor_stats_msg) == 16);

/* Vendor extension. */
struct ofp_vendor_header {
    struct ofp_header header;   /* Type OFPT_VENDOR. */
    ovs_be32 vendor;            /* Vendor ID:
                                 * - MSB 0: low-order bytes are IEEE OUI.
                                 * - MSB != 0: defined by OpenFlow
                                 *   consortium. */
    /* Vendor-defined arbitrary additional data. */
};
OFP_ASSERT(sizeof(struct ofp_vendor_header) == 12);

#endif /* openflow/openflow-1.0.h */
