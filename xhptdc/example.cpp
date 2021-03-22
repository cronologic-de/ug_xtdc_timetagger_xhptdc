// xhptdc8_user_guide_example.cpp : Example application for the xHPTDC8

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include "crono_interface.h"
#include "xHPTDC8_interface.h"

typedef unsigned int uint32;
typedef unsigned __int64 uint64;
int exit_on_fail(xhptdc8_manager hMgr, int status, const char* message);

// crate a manager object that provides access to all xHPTDC8 in the system
xhptdc8_manager initialize_xhptdc8(int buffer_size) {
	// prepare initialization
	xhptdc8_manager_init_parameters params;

	xhptdc8_get_default_init_parameters(&params);
	params.buffer_size = buffer_size;

	int error_code;
	char* error_msg;
	xhptdc8_manager hMgr;
	xhptdc8_init(&hMgr, &params, &error_code, (const char**)&error_msg); 
	exit_on_fail(hMgr, error_code, error_msg);
	return hMgr;
}

int get_device_count(xhptdc8_manager xhptdc8_man) {
	int error_code;
	char* error_msg;

	int device_count = xhptdc8_count_devices(&error_code, (const char**)&error_msg); 
	exit_on_fail(xhptdc8_man, error_code, error_msg);
	return device_count;
}

int configure_xhptdc8(xhptdc8_manager xhptdc8_man, int device_count) {
	xhptdc8_manager_configuration* mgr_cfg = new xhptdc8_manager_configuration;
	xhptdc8_get_default_configuration(xhptdc8_man, mgr_cfg);

	// configure all devices with an identical configuration
	for (int device_index = 0; device_index < device_count; device_index++) {
		for (int channel_index = 0; channel_index < XHPTDC8_TDC_CHANNEL_COUNT; channel_index++)
		{
			mgr_cfg->device_configs[device_index].trigger_threshold[channel_index] 
				= XHPTDC8_THRESHOLD_N_NIM;
			mgr_cfg->device_configs[device_index].channel[channel_index].enable = true;
		}
		mgr_cfg->device_configs[device_index].adc_channel.enable = 1;
		mgr_cfg->device_configs[device_index].adc_channel.watchdog_readout = 0;
		mgr_cfg->device_configs[device_index].adc_channel.trigger_threshold 
			= XHPTDC8_THRESHOLD_N_NIM;

		// configure an auto trigger at 150 kHz
		mgr_cfg->device_configs[device_index].auto_trigger_period = 1000;
		mgr_cfg->device_configs[device_index].auto_trigger_random_exponent = 0;

		// set all TiGers to create a short pulse for every auto trigger 
		for (int tiger_index = 0; tiger_index < XHPTDC8_TDC_CHANNEL_COUNT; tiger_index++)
		{
			mgr_cfg->device_configs[device_index].tiger_block[tiger_index].extend = false;
			mgr_cfg->device_configs[device_index].tiger_block[tiger_index].negate = false;
			mgr_cfg->device_configs[device_index].tiger_block[tiger_index].retrigger = false;
			mgr_cfg->device_configs[device_index].tiger_block[tiger_index].sources 
				= XHPTDC8_TRIGGER_SOURCE_AUTO;

			// every channel pulses a little later than the previous channel, for one clock cycle
			mgr_cfg->device_configs[device_index].gating_block[tiger_index].start = tiger_index * 2;
			mgr_cfg->device_configs[device_index].gating_block[tiger_index].start = tiger_index * 2 + 1;
		}
	}
	return xhptdc8_configure(xhptdc8_man, mgr_cfg);
}


void print_device_information(xhptdc8_manager xhptdc8_man) {
	xhptdc8_static_info staticinfo;
	printf("-------------------------\n");
	for (int i = 0; i < get_device_count(xhptdc8_man); i++) {
		xhptdc8_get_static_info(xhptdc8_man, i, &staticinfo);
		printf("Board Serial         : %d.%d\n", staticinfo.board_serial >> 24, staticinfo.board_serial & 0xffffff);
		printf("Board Configuration  : %d\n", staticinfo.board_configuration);
		printf("Board Revision       : %d\n", staticinfo.board_revision);
		printf("Firmware Revision    : %d.%d\n", staticinfo.firmware_revision, staticinfo.subversion_revision);
		printf("Driver Revision      : %d.%d.%d\n", ((staticinfo.driver_revision >> 16) & 255), ((staticinfo.driver_revision >> 8) & 255), (staticinfo.driver_revision & 255));
		printf("Driver SVN Revision  : %d\n", staticinfo.driver_build_revision);
	}
}

void print_hit(TDCHit* hit) {
	bool adc_data = ((hit->channel % 10) == 8) || ((hit->channel % 10) == 9);
	if (hit->type & XHPTDC8_TDCHIT_TYPE_ERROR)
		printf("Error:\n");

	printf("Channel %u - Time %llu - Type %x", hit->channel, hit->time, hit->type);
	if (adc_data)
		printf(" - ADC Data : %channel_index", (int)(hit->bin));

	printf("\n");
}

// call read_hits() once per millisecond until there is some data
int poll_for_hits(xhptdc8_manager xhptdc8_man, TDCHit* hit_buffer, size_t events_per_read) {
	while (true) {
		unsigned long hit_count = xhptdc8_read_hits(xhptdc8_man, hit_buffer, events_per_read);
		if (hit_count)
			return hit_count;
		Sleep(1);
	}
}


// fetch hits from the board by polling
void read_hits_wrapper(xhptdc8_manager xhptdc8_man, int events_per_read) {
	int total_event_count = events_per_read * 10000;

	TDCHit* hit_buffer = new TDCHit[events_per_read];

	int total_events = 0;
	while (total_events < total_event_count) {
		unsigned long hit_count = poll_for_hits(xhptdc8_man, hit_buffer, events_per_read);

		for (unsigned int i = 0; i < hit_count; i++)
		{
			TDCHit* hit = &(hit_buffer[i]);
			print_hit(hit);
			if ((total_events++ % 100) == 0)
				printf("Sum: %channel_index - Packet events: %channel_index\n", total_events, hit_count);
		}
	}

	delete hit_buffer;
}

// utility function to check for error, print error message and exit
int exit_on_fail(xhptdc8_manager x_man, int status, const char* message) {
	if (status == XHPTDC8_OK)
		return status;
	printf("%s: %s\n", message, xhptdc8_get_last_error_message(x_man));
	exit(1);
}

int main(int argc, char* argv[]) {
	printf("cronologic xhptdc8_user_guide_example using driver: %s\n", xhptdc8_get_driver_revision_str());

	xhptdc8_manager x_man = initialize_xhptdc8(8 * 1024 * 1024);

	exit_on_fail(x_man,
		configure_xhptdc8(x_man, get_device_count(x_man)),
		"Could not configure.");

	print_device_information(x_man);

	exit_on_fail(x_man,
		xhptdc8_start_capture(x_man),
		"Could not start capturing.");

	exit_on_fail(x_man,
		xhptdc8_start_tiger(x_man, 0),
		"Could not start TiGer.");

	read_hits_wrapper(x_man, 10000);

	exit_on_fail(x_man,
		xhptdc8_stop_capture(x_man),
		"Could not stop capturing.");

	exit_on_fail(x_man,
		xhptdc8_close(x_man),
		"Could not stop capturing.");

	return 0;
}