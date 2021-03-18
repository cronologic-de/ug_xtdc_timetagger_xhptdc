// xtdc8_user_guide_example.cpp : Example application for the xHPTDC8
#include "xtdc8_interface.h"
#include "stdio.h"
#include <windows.h>  // @Bassem: Is this needed? Can we go without?

typedef unsigned int uint32;
typedef unsigned __int64 uint64;


// crate a manager object that provides access to all xHPTDC8 in the system
xhptdc8_manager * initialize_xhptdc8(int buffer_size) {
	// prepare initialization
	xtdc8manager_init_parameters params;

	xhptdc8_get_default_init_parameters(&params);
	params.buffer_size = buffer_size;		

	int error_code;
	const char * error_msg;
	xhptdc8_manager *x_man = xhptdc8_init(&params , &error_code, &error_msg);
	exit_on_fail(x_man, error_code, error_msg);
	return x_man;
}

int get_device_count() {
	int error_code;
	const char * error_msg;

	int device_count = xhptdc8_count_devices(&error_code, &error_msg);
	exit_on_fail(x_man, error_code, error_msg);
	return device_count;
}


int configure_xhptdc8(xhptdc8_manager *xhptdc8_man, int device_count) {
	xhptdc8_manager_configuration *mgr_cfg = new xhptdc8_manager_configuration;

	// configure all devices with an identical configuration
	for (int j = 0; j < device_count; j++) {
		dconfig = new xhptdc8_device_configuration;
		xhptdc8_get_default_configuration(xhptdc8_man, j, dconfig);
		for (int i = 0; i < XHPTDC8_TDC_CHANNEL_COUNT; i++)
		{
			dconfig->tdc_trigger_offset[i] = XHPTDC8_THRESHOLD_N_NIM;
			dconfig->channel[i].enable = true;
		}
		dconfig->adc_channel.enable = 1;
		dconfig->adc_channel.watchdog_readout = 0;
		dconfig->adc_channel.trigger_offset = XHPTDC8_DC_OFFSET_N_NIM;

		// configure an auto trigger at 150 kHz
		dconfig->auto_trigger_period = 1000;
		dconfig->auto_trigger_random_exponent = 0;

		// set all TiGers to create a short pulse for every auto trigger 
		for (int i = 0; i < XHPTDC8_TDC_CHANNEL_COUNT; i++)
		{
			dconfig->tiger_block[i].extend = false;
			dconfig->tiger_block[i].negate = false;
			dconfig->tiger_block[i].retrigger = false;
			dconfig->tiger_block[i].sources = XHPTDC8_TRIGGER_SOURCE_AUTO;

			// every channel pulses a little later than the previous channel, for one clock cycle
			config->gating_block[i].start = i * 2;
			config->gating_block[i].start = i * 2 + 1;
		}
	}
	return xhptdc8_configure(xhptdc8_man, config);
}


void print_device_information(xhptdc8_manager *xhptdc8_man) {
	xhptdc8_static_info staticinfo;
	printf("-------------------------\n");
	for (int i = 0; i < get_device_count(); i++) {
		xhptdc8_get_static_info(xhptdc8_man, i, &staticinfo);
		printf("Board Serial         : %d.%d\n", staticinfo.board_serial >> 24, staticinfo.board_serial & 0xffffff);
		printf("Board Configuration  : %d\n", staticinfo.board_configuration);
		printf("Board Revision       : %d\n", staticinfo.board_revision);
		printf("Firmware Revision    : %d.%d\n", staticinfo.firmware_revision, staticinfo.subversion_revision);
		printf("Driver Revision      : %d.%d.%d\n", ((staticinfo.driver_revision >> 16) & 255), ((staticinfo.driver_revision >> 8) & 255), (staticinfo.driver_revision & 255));
		printf("Driver SVN Revision  : %d\n", staticinfo.driver_build_revision);
	}
}

void print_hit(TDCHit hit) {
	bool adc_data = ((hit.channel % 10) == 8) || ((hit.channel % 10) == 9);
	if (hit.type & XHPTDC8_TDCHIT_TYPE_ERROR)
		printf("Error:\n");

	printf("Channel %u - Time %llu - Type %x", hit.channel, hit.time, hit.type);
	if (adc_data)
		printf(" - ADC Data : %i", (int)(hit.bin));

	printf("\n");
}

// call read_hits() once per millisecond until there is some data
int poll_for_hits(xhptdc8_man, hit_buffer, events_per_read) {
	while (true) {
		unsigned long hit_count = xhptdc8_read_hits(xhptdc8_man, hit_buffer, events_per_read);
		if (hit_count) 
			return hit_count;
		Sleep(1);
	}
}


// fetch hits from the board by polling
void read_hits_wrapper(xhptdc8_manager *xhptdc8_man, int events_per_read) {
	int total_event_count(events_per_read * 10000);

	TDCHit* hit_buffer = new TDCHit[events_per_read];

	int total_events = 0;
	while (total_events < total_event_count) {
		unsigned long hit_count= poll_for_hits(xhptdc8_man, hit_buffer, events_per_read)

	for (unsigned int i = 0; i < hit_count; i++)
	{
		TDCHit hit = hit_buffer[i];
		print_hit(hit);			
		if ((total_events++ % 100) == 0)
			printf("Sum: %i - Packet events: %i\n", total_events, hit_count);
	}
}

// utility function to check for error, print error message and exit
void exit_on_fail(xhptdc8_manager x_man, int status, char* message) {
	if (status == XHPTDC8_OK)
		return status;
	printf("%s: %s\n", message, xhptdc8_get_last_error_message(x_man));
	exit(1);
}

int main(int argc, char* argv[]) {
	printf("cronologic xhptdc8_user_guide_example using driver: %s\n", xhptdc8_get_driver_revision_str());

	xhptdc8_manager * x_man = initialize_xhptdc8(8 * 1024 * 1024);

	exit_on_fail(x_man, 
		configure_xhptdc8(x_man, get_device_count()), 
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