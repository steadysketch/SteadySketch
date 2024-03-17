#include<core.p4>
#if __TARGET_TOFINO__ == 2
#include<t2na.p4>
#else
#include<tna.p4>
#endif

#define FILTER_SIZE 32768
#define BUCKET_SIZE 65536
#define FILTER_THRESHOLD 5
#define FREQ_DELTA_THRESHOLD 10
typedef bit<32> b32_t;
typedef bit<64> b64_t;
typedef bit<16> b16_t;
typedef bit<8> b8_t;


enum bit<16> ether_type_t {
    IPV4    = 0x0800,
    ARP     = 0x0806
}

enum bit<8> ip_proto_t {
    ICMP    = 1,
    IGMP    = 2,
    TCP     = 6,
    UDP     = 17
}
/************* HEADERS *************/
header ethernet_h {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> ether_type;
}

header ipv4_h {
    bit<4> version;
    bit<4> ihl;
    bit<8> diffserv;
    bit<16> total_len;
    bit<16> identification;
    bit<16> flags;
    bit<8> ttl;
    bit<8> protocol;
    bit<16> hdr_checksum;
    bit<32> src_addr;
    bit<32> dst_addr;
}

header meta_h {
    bit<16> filter_index_1;
    bit<16> filter_index_2;
    bit<16> bucket_index;
    bit<16> freq;
    bit<8> old_ts_1;
    bit<8> old_ts_2;
    bit<8> cur_ts;
    bit<8> cur_ts_add_one;
    bit<8> filter_state_1;
    bit<8> filter_state_2;
    bit<8> filter_counter_add_1;
    bit<8> filter_counter_add_2;
    bit<8> cur_filter_counter_1;
    bit<8> cur_filter_counter_2;
    bit<8> filter_pass_state;
    bit<8> filter_pass_counter;
}

header udp_h {
	bit<16> src_port;
	bit<16> dst_port;
	bit<16> total_len;
	bit<16> checksum;
}


struct ingress_header_t {
    ethernet_h ethernet;
    ipv4_h ipv4;
    udp_h udp;
    meta_h meta;
}

struct ingress_metadata_t{
    bit<16> max_freq;
    bit<16> min_freq;
    bit<16> freq_delta;
}

struct egress_header_t {
    ethernet_h ethernet;
    ipv4_h ipv4;
    udp_h udp;
}

struct egress_metadata_t {
}



/************* INGRESS *************/
parser IngressParser(packet_in pkt,
	out ingress_header_t hdr,
	out ingress_metadata_t meta,
	out ingress_intrinsic_metadata_t ig_intr_md)
{
	state start{
		pkt.extract(ig_intr_md);
		pkt.advance(PORT_METADATA_SIZE);
		transition parse_ethernet;
	}

	state parse_ethernet{
		pkt.extract(hdr.ethernet);
        transition select((bit<16>)hdr.ethernet.ether_type) {
            (bit<16>)ether_type_t.IPV4      : parse_ipv4;
            (bit<16>)ether_type_t.ARP       : accept;
            default : accept;
        }
	}

	state parse_ipv4{
		pkt.extract(hdr.ipv4);
        transition select(hdr.ipv4.protocol) {
            (bit<8>)ip_proto_t.ICMP             : accept;
            (bit<8>)ip_proto_t.IGMP             : accept;
            (bit<8>)ip_proto_t.TCP              : accept;
            (bit<8>)ip_proto_t.UDP              : parse_meta;
            default : accept;
        }
	}

	state parse_meta{
        pkt.extract(hdr.udp);
        hdr.meta.setValid();
		transition accept;
	}
}


control Ingress(inout ingress_header_t hdr,
		inout ingress_metadata_t meta,
		in ingress_intrinsic_metadata_t ig_intr_md,
		in ingress_intrinsic_metadata_from_parser_t ig_prsr_md,
		inout ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md,
		inout ingress_intrinsic_metadata_for_tm_t ig_tm_md)
{
	Hash<b16_t>(HashAlgorithm_t.IDENTITY) hash_1;
    Hash<b16_t>(HashAlgorithm_t.CRC32) hash_2;
    CRCPolynomial<bit<32>>(coeff=0x04C11DB7,reversed=true, msb=false, extended=false, init=0xFFFFFFFF, xor=0xFFFFFFFF) crc32_1;
	Hash<b16_t>(HashAlgorithm_t.CUSTOM, crc32_1) hash_3;

    action preprocessing() {
        hdr.meta.filter_index_1 = hash_1.get({hdr.ipv4.src_addr, hdr.ipv4.dst_addr});
        hdr.meta.filter_index_2 = hash_2.get({hdr.ipv4.src_addr, hdr.ipv4.dst_addr});
    }

    @stage(0) 
    table preprocessing_table{
        actions = {
            preprocessing;
        }
        size = 1;
        const default_action = preprocessing;
    }

    action preprocessing_1() {
        hdr.meta.bucket_index = hash_3.get({hdr.ipv4.src_addr, hdr.ipv4.dst_addr});
        hdr.meta.cur_ts = ig_intr_md.ingress_mac_tstamp[23:16];
    }

    @stage(0) 
    table preprocessing_table_1{
        actions = {
            preprocessing_1;
        }
        size = 1;
        const default_action = preprocessing_1;
    }

    //FILTER TIMESTAMP
    Register<b8_t, _>(FILTER_SIZE) ts_1;
    RegisterAction<b8_t, _, b8_t>(ts_1) ts_update_1_salu = {
        void apply(inout bit<8> reg_data, out bit<8> out_data) {
            out_data = reg_data;
            reg_data = hdr.meta.cur_ts;
        }
    };

    action ts_update_action_1() {
        hdr.meta.old_ts_1 = ts_update_1_salu.execute(hdr.meta.filter_index_1);
    }

    @stage(1)
    table ts_update_table_1{
        actions = {
            ts_update_action_1;
        }
        size = 1;
        const default_action = ts_update_action_1;
    }

    Register<b8_t, _>(FILTER_SIZE) ts_2;
    RegisterAction<b8_t, _, b8_t>(ts_2) ts_update_2_salu = {
        void apply(inout bit<8> reg_data, out bit<8> out_data) {
            out_data = reg_data;
            reg_data = hdr.meta.cur_ts;
        }
    };

    action ts_update_action_2() {
        hdr.meta.old_ts_2 = ts_update_2_salu.execute(hdr.meta.filter_index_2);
    }

    @stage(1)
    table ts_update_table_2{
        actions = {
            ts_update_action_2;
        }
        size = 1;
        const default_action = ts_update_action_2;
    }

    action add_ts_one() {
        hdr.meta.cur_ts_add_one = hdr.meta.cur_ts + 1;
    }

    @stage(1)
    table add_ts_one_table{
        actions = {
            add_ts_one;
        }
        size = 1;
        const default_action = add_ts_one;
    }

    //FILTER COUNTER
    Register<b8_t, _>(FILTER_SIZE) counter_1;
    RegisterAction<b8_t, _, b8_t>(counter_1) counter_update_1_salu = {
        void apply(inout bit<8> reg_data, out bit<8> out_data) {
            if(hdr.meta.filter_state_1 == 1) {
                reg_data = reg_data + hdr.meta.filter_counter_add_1;
            }
            else {
                reg_data = 1;
            }
            out_data = reg_data;
        }
    };

    action counter_update_1_action() {
        hdr.meta.cur_filter_counter_1 = counter_update_1_salu.execute(hdr.meta.filter_index_1);
    }

    @stage(3)
    table counter_update_table_1{
        actions = {
            counter_update_1_action;
        }
        size = 1;
        const default_action = counter_update_1_action;
    }

    Register<b8_t, _>(FILTER_SIZE) counter_2;
    RegisterAction<b8_t, _, b8_t>(counter_2) counter_update_2_salu = {
        void apply(inout bit<8> reg_data, out bit<8> out_data) {
            if(hdr.meta.filter_state_2 == 1) {
                reg_data = reg_data + hdr.meta.filter_counter_add_2;
            }
            else {
                reg_data = 1;
            }
            out_data = reg_data;
        }
    };

    action counter_update_2_action() {
        hdr.meta.cur_filter_counter_2 = counter_update_2_salu.execute(hdr.meta.filter_index_2);
    }

    @stage(3)
    table counter_update_table_2{
        actions = {
            counter_update_2_action;
        }
        size = 1;
        const default_action = counter_update_2_action;
    }

    //rolling sketch
    Register<b16_t, _>(BUCKET_SIZE) freq;
    RegisterAction<b16_t, _, b16_t>(freq) freq_update_salu = {
        void apply(inout bit<16> reg_data, out bit<16> out_data) {
            out_data = reg_data;
            if(hdr.meta.filter_pass_state == 0) {
                reg_data = reg_data + 1;
            }
            else {
                reg_data = 1;
            }
        }
    };

    action freq_update_action() {
        hdr.meta.freq = freq_update_salu.execute(hdr.meta.bucket_index);
    }

    @stage(5)
    table freq_update_table{
        actions = {
            freq_update_action;
        }
        size = 1;
        const default_action = freq_update_action;
    }

    Register<b16_t, _>(BUCKET_SIZE) max_freq;
    RegisterAction<b16_t, _, b16_t>(max_freq) max_freq_update_salu = {
        void apply(inout bit<16> reg_data, out bit<16> out_data) {
            if(reg_data < hdr.meta.freq) {
                reg_data = hdr.meta.freq;
            }
            out_data = reg_data;
        }
    };

    action max_freq_update_action() {
        meta.max_freq = max_freq_update_salu.execute(hdr.meta.bucket_index);
    }

    @stage(6)
    table max_freq_update_table{
        actions = {
            max_freq_update_action;
        }
        size = 1;
        const default_action = max_freq_update_action;
    }

    RegisterAction<b16_t, _, b16_t>(max_freq) max_freq_clear_salu = {
        void apply(inout bit<16> reg_data, out bit<16> out_data) {
            reg_data = 0;
            out_data = reg_data;
        }
    };

    action max_freq_clear_action() {
        max_freq_clear_salu.execute(hdr.meta.bucket_index);
    }

    @stage(6)
    table max_freq_clear_table{
        actions = {
            max_freq_clear_action;
        }
        size = 1;
        const default_action = max_freq_clear_action;
    }

    RegisterAction<b16_t, _, b16_t>(max_freq) max_freq_read_salu = {
        void apply(inout bit<16> reg_data, out bit<16> out_data) {
            out_data = reg_data;
        }
    };

    action max_freq_read_action() {
        meta.max_freq = max_freq_read_salu.execute(hdr.meta.bucket_index);
    }

    @stage(6)
    table max_freq_read_table{
        actions = {
            max_freq_read_action;
        }
        size = 1;
        const default_action = max_freq_read_action;
    }

    Register<b16_t, _>(BUCKET_SIZE) min_freq;
    RegisterAction<b16_t, _, b16_t>(min_freq) min_freq_update_salu = {
        void apply(inout bit<16> reg_data, out bit<16> out_data) {
            if(reg_data > hdr.meta.freq) {
                reg_data = hdr.meta.freq;
            }
            out_data = reg_data;
        }
    };

    action min_freq_update_action() {
        meta.min_freq = min_freq_update_salu.execute(hdr.meta.bucket_index);
    }

    @stage(6)
    table min_freq_update_table{
        actions = {
            min_freq_update_action;
        }
        size = 1;
        const default_action = min_freq_update_action;
    }

    RegisterAction<b16_t, _, b16_t>(min_freq) min_freq_clear_salu = {
        void apply(inout bit<16> reg_data, out bit<16> out_data) {
            reg_data = 0x7fff;
            out_data = reg_data;
        }
    };

    action min_freq_clear_action() {
        min_freq_clear_salu.execute(hdr.meta.bucket_index);
    }

    @stage(6)
    table min_freq_clear_table{
        actions = {
            min_freq_clear_action;
        }
        size = 1;
        const default_action = min_freq_clear_action;
    }

    RegisterAction<b16_t, _, b16_t>(min_freq) min_freq_read_salu = {
        void apply(inout bit<16> reg_data, out bit<16> out_data) {
            out_data = reg_data;
        }
    };

    action min_freq_read_action() {
        meta.min_freq = min_freq_read_salu.execute(hdr.meta.bucket_index);
    }

    @stage(6)
    table min_freq_read_table{
        actions = {
            min_freq_read_action;
        }
        size = 1;
        const default_action = min_freq_read_action;
    }

    action add_min_freq() {
        meta.min_freq = meta.min_freq + FREQ_DELTA_THRESHOLD;
    }

    @stage(7)
    table add_min_freq_table{
        actions = {
            add_min_freq;
        }
        size = 1;
        const default_action = add_min_freq;
    }

	Register<b16_t, _>(1) find_min_freq;
	RegisterAction<b16_t, _, b16_t>(find_min_freq) find_min_freq_salu = {
		void apply(inout b16_t reg_data, out b16_t out_data) {
			if(meta.freq_delta[15:15] == 1) {
				out_data = 0;
			}
			else {
				out_data = 1;
			}
		}
	};

	action find_min_freq_action() {
		meta.freq_delta = find_min_freq_salu.execute(0);
	}

	table find_min_freq_table {
		actions = {
			find_min_freq_action;
		}
		size = 1;
		const default_action = find_min_freq_action;
	}

    apply {
        preprocessing_table.apply();
        preprocessing_table_1.apply();
        ts_update_table_1.apply();
        ts_update_table_2.apply();
        add_ts_one_table.apply();
        if(hdr.meta.cur_ts == hdr.meta.old_ts_1) {
            hdr.meta.filter_state_1 = 0;
            hdr.meta.filter_counter_add_1 = 0;
        }
        else if(hdr.meta.cur_ts == hdr.meta.cur_ts_add_one) {
            hdr.meta.filter_state_1 = 1;
            hdr.meta.filter_counter_add_1 = 1;
        }
        else {
            hdr.meta.filter_state_1 = 2;
        }

        if(hdr.meta.cur_ts == hdr.meta.old_ts_2) {
            hdr.meta.filter_state_2 = 0;
            hdr.meta.filter_counter_add_2 = 0;
        }
        else if(hdr.meta.cur_ts == hdr.meta.cur_ts_add_one) {
            hdr.meta.filter_state_2 = 1;
            hdr.meta.filter_counter_add_2 = 1;
        }
        else {
            hdr.meta.filter_state_2 = 2;
        }

        counter_update_table_1.apply();
        counter_update_table_2.apply();
        hdr.meta.filter_pass_state = max(hdr.meta.filter_state_1, hdr.meta.filter_state_2);
        hdr.meta.filter_pass_counter = min(hdr.meta.cur_filter_counter_1, hdr.meta.cur_filter_counter_2);

        freq_update_table.apply();

        if(hdr.meta.filter_pass_state == 0) {
            max_freq_read_table.apply();
            min_freq_read_table.apply();
        }
        else if(hdr.meta.filter_pass_state == 1) {
            max_freq_update_table.apply();
            min_freq_update_table.apply();
        }
        else if(hdr.meta.filter_pass_state == 2) {
            max_freq_clear_table.apply();
            min_freq_clear_table.apply();
        }

        if(hdr.meta.filter_pass_counter >= FILTER_THRESHOLD) {
            add_min_freq_table.apply();
            meta.freq_delta = meta.max_freq - meta.min_freq;
            find_min_freq_table.apply();
            if(meta.freq_delta == 0 && hdr.meta.filter_pass_state < 2) {
                ig_dprsr_md.drop_ctl = 0;   //steady sketch
            }
            else {
                ig_dprsr_md.drop_ctl = 1;
            }
        }
        else {
			ig_dprsr_md.drop_ctl = 1;
        }

        hdr.meta.setInvalid();
    }

}

control IngressDeparser(packet_out pkt,
	inout ingress_header_t hdr,
	in ingress_metadata_t meta,
	in ingress_intrinsic_metadata_for_deparser_t ig_dprtr_md)
{
	apply{
		pkt.emit(hdr);
	}
}


/************* INGRESS *************/
parser EgressParser(packet_in pkt,
	out egress_header_t hdr,
	out egress_metadata_t meta,
	out egress_intrinsic_metadata_t eg_intr_md)
{
	state start{
		pkt.extract(eg_intr_md);
		transition parse_ethernet;
	}

	state parse_ethernet{
		pkt.extract(hdr.ethernet);
        transition select((bit<16>)hdr.ethernet.ether_type) {
            (bit<16>)ether_type_t.IPV4      : parse_ipv4;
            (bit<16>)ether_type_t.ARP       : accept;
            default : accept;
        }
	}

	state parse_ipv4{
		pkt.extract(hdr.ipv4);
        transition select(hdr.ipv4.protocol) {
            (bit<8>)ip_proto_t.ICMP             : accept;
            (bit<8>)ip_proto_t.IGMP             : accept;
            (bit<8>)ip_proto_t.TCP              : accept;
            (bit<8>)ip_proto_t.UDP              : parse_udp;
            default : accept;
        }
	}

	state parse_udp{
        pkt.extract(hdr.udp);
		transition accept;
	}
}

control Egress(inout egress_header_t hdr,
	inout egress_metadata_t meta,
	in egress_intrinsic_metadata_t eg_intr_md,
	in egress_intrinsic_metadata_from_parser_t eg_prsr_md,
	inout egress_intrinsic_metadata_for_deparser_t eg_dprsr_md,
	inout egress_intrinsic_metadata_for_output_port_t eg_oport_md)
{
    apply {

    }
}

control EgressDeparser(packet_out pkt,
	inout egress_header_t hdr,
	in egress_metadata_t meta,
	in egress_intrinsic_metadata_for_deparser_t eg_dprsr_md)
{
	apply{
        pkt.emit(hdr);
	}
}


/* main */
Pipeline(IngressParser(),Ingress(),IngressDeparser(),
EgressParser(),Egress(),EgressDeparser()) pipe;

Switch(pipe) main;