// timetagger4_user_guide_example.cpp : Example application for the TimeTagger4
#include "timetagger4_interface.h"
#include "stdio.h"
#include <windows.h>

typedef unsigned int uint32;
typedef unsigned __int64 uint64;

timetagger4_device * initialize_timetagger(int buffer_size, int board_id, int card_index) {
	// prepare initialization
	timetagger4_init_parameters params;

	timetagger4_get_default_init_parameters(&params);
	params.buffer_size[0] = buffer_size;				// size of the packet buffer
	params.board_id = board_id;							// value copied to "card" field of every packet, allowed range 0..255
	params.card_index = card_index;						// which of the TimeTagger4 board found in the system to be used

	int error_code;
	const char * err_message;
	timetagger4_device * device = timetagger4_init(&params, &error_code, &err_message);
	if (error_code != CRONO_OK) {
		printf("Could not init TimeTagger4 compatible board: %s\n", err_message);
		return nullptr;
	}
	return device;
}

int configure_timetagger(timetagger4_device * device) {
	// prepare configuration
	timetagger4_configuration config;

	// fill configuration data structure with default values
	// so that the configuration is valid and only parameters
	// of interest have to be set explicitly
	timetagger4_get_default_configuration(device, &config);

	// set config of the 4 TDC channels
	for (int i = 0; i < TIMETAGGER4_TDC_CHANNEL_COUNT; i++)
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
	config.auto_trigger_period = 1250;  // multiples of 4 ns
	config.auto_trigger_random_exponent = 0;

	// setup TiGeR
	// sending a signal to the LEMO outputs (and to the TDC on the same channel)
	// requires proper 50 Ohm termination on the LEMO output to work reliably

	// use 200 kHz auto trigger to generate
	// a 200 kHz signal with 12 ns pulse width on LEMO output Start
	config.tiger_block[0].enable = 1;
	config.tiger_block[0].start = 0;
	config.tiger_block[0].stop = config.tiger_block[0].start + 2;
	config.tiger_block[0].negate = 0;
	config.tiger_block[0].retrigger = 0;
	config.tiger_block[0].extend = 0;
	config.tiger_block[0].enable_lemo_output = 1;
	config.tiger_block[0].sources = TIMETAGGER4_TRIGGER_SOURCE_AUTO;

	// if TiGeR is used for triggering with positive pulses
	if (config.tiger_block[0].enable == 1)
		config.dc_offset[0] = TIMETAGGER4_DC_OFFSET_P_LVCMOS_18;
	else
		config.dc_offset[0] = TIMETAGGER4_DC_OFFSET_N_NIM;

	// write configuration to board
	return timetagger4_configure(device, &config);
}

double get_binsize(timetagger4_device * device) {
	timetagger4_param_info parinfo;
	timetagger4_get_param_info(device, &parinfo);
	return parinfo.binsize;
}

void print_device_information(timetagger4_device * device) {
	// print board information
	timetagger4_static_info staticinfo;
	timetagger4_get_static_info(device, &staticinfo);
	printf("Board Serial        : %d.%d\n", staticinfo.board_serial >> 24, staticinfo.board_serial & 0xffffff);
	printf("Board Configuration : TimeTagger4-%iG\n", staticinfo.chip_id);
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

_int64 processPacket(_int64 group_abs_time_old, volatile crono_packet *p, int update_count, double binsize) {
	// do something with the data, e.g. calculate current rate
	_int64 group_abs_time = p->timestamp;
	// group timestamp increments at 2 GHz
	double rate = (2e9 / ((double)(group_abs_time - group_abs_time_old) / (double)update_count));
	printf("\r%.2f kHz", rate / 1000.0);

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
	printf("cronologic timetagger4_user_guide_example using driver: %s\n", timetagger4_get_driver_revision_str());

	timetagger4_device * device = initialize_timetagger(8 * 1024 * 1024, 0, 0);

	int status = configure_timetagger(device);
	if (status != CRONO_OK) {
		printf("Could not configure TimeTagger4: %s", timetagger4_get_last_error_message(device));
		timetagger4_close(device);
		return status;
	}

	print_device_information(device);

	// configure readout behaviour
	timetagger4_read_in read_config;
	// automatically acknowledge all data as processed
	// on the next call to timetagger4_read()
	// old packet pointers are invalid after calling timetagger4_read()
	read_config.acknowledge_last_read = 1;

	// structure with packet pointers for read data
	timetagger4_read_out read_data;

	// start data capture
	status = timetagger4_start_capture(device);
	if (status != CRONO_OK) {
		printf("Could not start capturing %s", timetagger4_get_last_error_message(device));
		timetagger4_close(device);
		return status;
	}

	// start timing generator
	timetagger4_start_tiger(device);

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
		status = timetagger4_read(device, &read_config, &read_data);
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
					group_abs_time = processPacket(group_abs_time, p, update_count, binsize);
				}
				p = crono_next_packet(p);
				packet_count++;
			}
		}
	}

	// shut down packet generation and DMA transfers
	timetagger4_stop_capture(device);

	// deactivate timetagger4
	timetagger4_close(device);

	return 0;
}