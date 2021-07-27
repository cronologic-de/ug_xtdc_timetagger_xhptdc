// xtdc4_user_guide_example.cpp : Example application for the xTDC4
#include "xTDC4_interface.h"
#include "stdio.h"
#include <windows.h>

typedef unsigned int uint32;
typedef unsigned __int64 uint64;

xtdc4_device * initialize_xtdc4(int buffer_size, int board_id, int card_index) {
	// prepare initialization
	xtdc4_init_parameters params;

	xtdc4_get_default_init_parameters(&params);
	params.buffer_size[0] = buffer_size;		// size of the packet buffer
	params.board_id = board_id;					// value copied to "card" field of every packet, allowed range 0..255
	params.card_index = card_index;				// which of the xTDC4 board found in the system to be used

	int error_code;
	const char * err_message;
	xtdc4_device * device = xtdc4_init(&params, &error_code, &err_message);
	if (error_code != CRONO_OK) {
		printf("Could not init xTDC4 compatible board: %s\n", err_message);
		return nullptr;
	}
	return device;
}

int configure_xtdc4(xtdc4_device * device) {
	// prepare configuration
	xtdc4_configuration config;

	// fill configuration data structure with default values
	// so that the configuration is valid and only parameters
	// of interest have to be set explicitly
	xtdc4_get_default_configuration(device, &config);

	// set config of the 4 TDC channels
	for (int i = 0; i < XTDC4_TDC_CHANNEL_COUNT; i++)
	{
		// enable recording hits on TDC channel
		config.channel[i].enabled = true;

		// define range of the group
		config.channel[i].start = 0;	// range begins right after start pulse
		config.channel[i].stop = 30000;	// recording window stops after ~15 us

		// measure only falling edge
		config.trigger[i + 1].falling = 1;
		config.trigger[i + 1].rising = 0;
	}

	// start group on falling edges on the start channel 0
	config.trigger[0].falling = 1;	// enable packet generation on falling edge of start pulse
	config.trigger[0].rising = 0;	// disable packet generation on rising edge of start pulse

	// generate an internal 200 kHz trigger
	config.auto_trigger_period = 750;           // multiples of 6.666 ns
	config.auto_trigger_random_exponent = 0;

	// setup TiGeR
	// sending a signal to the LEMO outputs (and to the TDC on the same channel)
	// requires proper 50 Ohm termination on the LEMO output to work reliably

	// use 200 kHz auto trigger to generate
	// a 200 kHz signal with 12 ns pulse width on LEMO output Start
	config.tiger_block[0].enable = 1;
	config.tiger_block[0].start = 0;
	config.tiger_block[0].stop = config.tiger_block[0].start + 1;
	config.tiger_block[0].negate = 0;
	config.tiger_block[0].retrigger = 0;
	config.tiger_block[0].extend = 0;
	config.tiger_block[0].enable_lemo_output = 1;
	config.tiger_block[0].sources = XTDC4_TRIGGER_SOURCE_AUTO;

	// if TiGeR is used for triggering with positive pulses
	config.dc_offset[0] = XTDC4_DC_OFFSET_P_LVCMOS_18;

	// write configuration to board
	return xtdc4_configure(device, &config);
}

double get_binsize(xtdc4_device * device) {
	xtdc4_param_info parinfo;
	xtdc4_get_param_info(device, &parinfo);
	return parinfo.binsize;
}

void print_device_information(xtdc4_device * device) {
	// print board information
	xtdc4_static_info staticinfo;
	xtdc4_get_static_info(device, &staticinfo);
	printf("Board Serial        : %d.%d\n", staticinfo.board_serial >> 24, staticinfo.board_serial & 0xffffff);
	printf("Board Configuration : 0x%x\n", staticinfo.board_configuration);
	printf("Board Revision      : %d\n", staticinfo.board_revision);
	printf("Firmware Revision   : %d.%d\n", staticinfo.firmware_revision, staticinfo.subversion_revision);
	printf("Driver Revision     : %d.%d.%d\n", ((staticinfo.driver_revision >> 16) & 255), ((staticinfo.driver_revision >> 8) & 255), (staticinfo.driver_revision & 255));
	printf("Driver SVN Revision : %d\n", staticinfo.driver_build_revision);
	printf("\nTDC binsize         : %0.2f ps\n", get_binsize(device));
}

void print_hit(uint32 hit, double binsize) {
	// extract channel number (A-D)
	char channel = 65 + (hit & 0xf);

	// extract hit flags
	int flags = (hit >> 4 & 0xf);

	// extract hit timestamp
	int ts_offset = (hit >> 8 & 0xffffff);

	// TDC bin size is 500 ps. Convert timestamp to ns.
	double ts_offset_ns = ts_offset;
	ts_offset_ns *= binsize / 1000.0;

	printf("Hit @Channel %c - Flags %d - Offset %u (raw) / %.1f ns\n", channel, flags, ts_offset, ts_offset_ns);
}

_int64 process_packet(_int64 group_abs_time_old, volatile crono_packet *p, int update_count, double binsize) {
	// do something with the data, e.g. calculate current rate
	_int64 group_abs_time = p->timestamp;
	// group timestamp increments at 600 MHz
	double rate = (600000000 / ((double)(group_abs_time - group_abs_time_old) / (double)update_count));
	printf("\r%.2f kHz ", rate / 1000.0);

	// ...or print hits (not a good idea at high data rates,
	printf("Card %d - Flags %d - Length %d - Type %d - TS %llu\n", p->card, p->flags, p->length, p->type, p->timestamp);

	// There fit two hits into every 64 bit word. 
	// The flag with weight 1 tells us, whether the number of hits in the packet is odd
	int hit_count = 2 * (p->length);
	if ((p->flags & 0x1) == 1)
		hit_count -= 1;

	uint32* packet_data = (uint32*)(p->data);
	for (int i = 0; i < hit_count; i++)
	{
		print_hit(packet_data[i], binsize);
	}
	printf("\n\n");
	return group_abs_time;
}

int main(int argc, char* argv[]) {
	printf("cronologic xtdc4_user_guide_example using driver: %s\n", xtdc4_get_driver_revision_str());

	xtdc4_device * device = initialize_xtdc4(8 * 1024 * 1024, 0, 0);

	int status = configure_xtdc4(device);
	if (status != CRONO_OK) {
		printf("Could not configure xTDC4: %s", xtdc4_get_last_error_message(device));
		xtdc4_close(device);
		return status;
	}

	print_device_information(device);

	// configure readout behaviour
	xtdc4_read_in read_config;
	// automatically acknowledge all data as processed
	// on the next call to xtdc4_read()
	// old packet pointers are invalid after calling xtdc4_read()
	read_config.acknowledge_last_read = 1;

	// structure with packet pointers for read data
	xtdc4_read_out read_data;

	// start data capture
	status = xtdc4_start_capture(device);
	if (status != CRONO_OK) {
		printf("Could not start capturing %s", xtdc4_get_last_error_message(device));
		xtdc4_close(device);
		return status;
	}

	// start timing generator
	xtdc4_start_tiger(device);

	// some book keeping
	int packet_count = 0;
	int empty_packets = 0;
	int packets_with_errors = 0;
	bool last_read_no_data = false;

	_int64 group_abs_time = 0;
	_int64 group_abs_time_old = 0;
	int update_count = 100;
	double binsize = get_binsize(device);

	printf("Reading packets:\n");
	// read 10000 packets
	while (packet_count < 10000)
	{
		// get pointers to acquired packets
		status = xtdc4_read(device, &read_config, &read_data);
		if (status != CRONO_OK) {
			Sleep(100);
			printf(" - No data! -\n");
		}
		else
		{
			// iterate over all packets received with the last read
			volatile crono_packet* p = read_data.first_packet;
			while (p <= read_data.last_packet)
			{
				// printf is slow, so this demo only processes every 1000th packet
				// your application would of course collect every packet
				if (packet_count % update_count == 0) {
					group_abs_time = process_packet(group_abs_time, p, update_count, binsize);
				}
				p = crono_next_packet(p);
				packet_count++;
			}
		}
	}

	// shut down packet generation and DMA transfers
	xtdc4_stop_capture(device);

	// deactivate xTDC4
	xtdc4_close(device);

	return 0;
}