// xtdc8_user_guide_example.cpp : Example application for the xHPTDC8
#include "xtdc8_interface.h"
#include "xtdc8_grouping.h"
#include "stdio.h"
#include <windows.h>

typedef unsigned int uint32;
typedef unsigned __int64 uint64;


xhptdc8_manager * initialize_xhptdc8(int buffer_size) {
	// prepare initialization
	xtdc8manager_init_parameters params;

	xhptdc8_get_default_init_parameters(&params);
	params.buffer_size = buffer_size;		

	int error_code;
	const char * err_message;
	xhptdc8_manager *x_man = xhptdc8_init(&params , &error_code, &err_message);
	if (error_code != CRONO_OK)
	{
		printf("Could not initialize xHPTDC8 compatible board: %s\n", err_message);
		exit(1);
	}
	return x_man;
}


int get_device_count() {
	int error_code;
	const char * error_message;

	int device_count = xhptdc8_count_devices(&error_code, &error_message);
	if (error_code != CRONO_OK)
	{
		printf("Could not count xHPTDC8: %s\n", error_message);
		exit(1);
	}
	return device_count;
}


//@TO-Do bin mir noch unsicher welche config sinvoll ist
int configure_xhptdc8(xhptdc8_manager *xhptdc8_man, int device_count) {
	xhptdc8_manager_configuration *mgr_cfg = new xhptdc8_manager_configuration;
	for (int j = 0; j < device_count; j++) {
		config[j] = new xhptdc8_device_configuration;
		xhptdc8_get_default_configuration(xhptdc8_man, j, config[j]);
		for (int i = 0; i < XHPTDC8_TDC_CHANNEL_COUNT; i++)
		{
			config[j]->tdc_trigger_offset[i] = XHPTDC8_DC_OFFSET_N_NIM;
			config[j]->channel[i].enable = true;
		}
		config[j]->adc_channel.enable = 1;
		config[j]->adc_channel.watchdog_readout = 0;
		config[j]->adc_channel.trigger_offset = XHPTDC8_DC_OFFSET_N_NIM;

		config[j]->tdc_mode = XHPTDC8_TDC_MODE_CONTINUOUS;

		config[j]->auto_trigger_period = 1000;
		config[j]->auto_trigger_random_exponent = 0;

		for (int i = 0; i < XHPTDC8_TDC_CHANNEL_COUNT; i++)
		{
			config[j]->gating_block[i].extend = false;
			config[j]->gating_block[i].negate = false;
			config[j]->gating_block[i].retrigger = false;
			config[j]->gating_block[i].sources = XHPTDC8_TRIGGER_SOURCE_AUTO;

			if (i == 0)
				config[j]->gating_block[i].start = 0;
			else
				config[j]->gating_block[i].start = 3;
			config[j]->gating_block[i].stop = config[j]->gating_block[i].start + 1;
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

//@TO-Do hier sind auch noch anpassungen noetig
void read_hits_wrapper(xhptdc8_manager *xhptdc8_man, int events_per_read) {
	int total_event_count(events_per_read * 10000);

	TDCHit* hit_buffer = new TDCHit[events_per_read];

	unsigned long long last_ts = 0;
	int total_events = 0;

	while (total_events < total_event_count) {
		unsigned long hit_count = 0;

		while (true) {
			hit_count = xhptdc8_read_hits(xhptdc8_man, hit_buffer, events_per_read);
			if (!hit_count) {
				Sleep(1);
				continue;
			}
			break;
		}

		if (hit_count > 0)
		{
			//total_events += hit_count;

			unsigned long long last_i_ts = 0;
			for (unsigned int i = 0; i < hit_count; i++)
			{
				bool adc_data = ((hit_buffer[i].channel % 10) == 8) || ((hit_buffer[i].channel % 10) == 9);
				if ((hit_buffer[i].type & XHPTDC8_TDCHIT_TYPE_ERROR) > 0)
				{
					printf("Error!\n");
					printf("Channel  : %u\n", hit_buffer[i].channel);
					printf("Type     : %x\n", (byte)hit_buffer[i].type);
					printf("Time     : %llu\n", hit_buffer[i].time);
					if (adc_data)
						printf("ADC Data : %i\n", (int)(hit_buffer[i].bin));
				}
				if (last_ts > hit_buffer[i].time)
				{
					printf("Events not sorted!\n");
					printf("Channel: %u\n", hit_buffer[i].channel);
					printf("Type  : %x\n", (byte)hit_buffer[i].type);
					printf("Time : %llu\n", hit_buffer[i].time);
					if (adc_data)
						printf("ADC Data : %i\n", (int)(hit_buffer[i].bin));
				}
				last_ts = hit_buffer[i].time;
				printf("Channel %u - Time %llu - Type %x", hit_buffer[i].channel, hit_buffer[i].time, hit_buffer[i].type);
				if (adc_data)
					printf(" - ADC Data : %i", (int)(hit_buffer[i].bin));

				printf("\n");
				total_events++;
				if ((total_events % 100) == 0)
					printf("Sum: %i - Packet events: %i\n", total_events, hit_count);
			}
		}
	}
}


int main(int argc, char* argv[]) {
	printf("cronologic xhptdc8_user_guide_example using driver: %s\n", xhptdc8_get_driver_revision_str());

	xhptdc8_manager * x_man = initialize_xhptdc8(8 * 1024 * 1024);

	int status = configure_xhptdc8(x_man, get_device_count());
	if (status != CRONO_OK)
	{
		printf("Could not configure xHPTDC8: %s\n", xhptdc8_get_last_error_message(x_man));
		exit(1);
	}

	print_device_information(x_man);

	status = xhptdc8_start_capture(x_man);
	if (status != CRONO_OK)
	{
		printf("Could not start capturing: %s\n", xhptdc8_get_last_error_message(x_man));
		exit(1);
	}

	status = xhptdc8_start_tiger(x_man, 0);
	if (status != CRONO_OK)
	{
		printf("Could not start TiGer: %s\n", xhptdc8_get_last_error_message(x_man));
		exit(1);
	}

	read_hits_wrapper(x_man, 10000);

	status = xhptdc8_stop_capture(x_man);
	if (status != CRONO_OK)
	{
		printf("Could not stop capturing: %s\n", xhptdc8_get_last_error_message(x_man));
		exit(1);
	}

	status = xhptdc8_close(x_man);
	if (status != CRONO_OK)
	{
		printf("Error closing xHPTDC8: %s\n", xhptdc8_get_last_error_message(x_man));
		exit(1);
	}
	return 0;
}