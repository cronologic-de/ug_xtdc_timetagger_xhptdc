// timetagger4_user_guide_example.cpp : Example application for the TimeTagger4
#include <stdio.h>
#include <stdlib.h>
#include <chrono>
#include <thread>
#include "TimeTagger4_interface.h"

// If true the time tagger triggers a start periodically
// The time difference of signals on channel A are measured
// else start signal either from input or tiger is used (see below)
// frequency of  start signal is printed and the hits are sampled
const bool USE_CONTINUOUS_MODE = false;
const bool USE_TIGER_START = true; // if false, external signal must be
                                    // provided on start; not applicable if
                                    // continuous mode is enabled
const bool USE_TIGER_STOPS = true;  // if false please connect signals to
                                    // some of channels A-D


timetagger4_device* initialize_timetagger(int buffer_size,
                                          int board_id,
                                          int card_index){
    // prepare initialization
    timetagger4_init_parameters params;

    timetagger4_get_default_init_parameters(&params);
    params.buffer_size[0] = buffer_size; // size of the packet buffer
    params.board_id = board_id;          // value copied to "card" field of
                                         // every packet, allowed range 0..255
    params.card_index = card_index;      // which of the TimeTagger4 board 
                                         // found in the system to be used
    int error_code;
    const char * err_message;
    timetagger4_device* device = timetagger4_init(&params,
                                                  &error_code,
                                                  &err_message);
    if (error_code != CRONO_OK) {
        printf("Could not init TimeTagger4 compatible board: %s\n",
               err_message);
        return nullptr;
    }
    timetagger4_static_info static_info;
    timetagger4_get_static_info(device, &static_info);
    bool timeTaggerNG = static_info.board_revision >= 7;
    if (USE_CONTINUOUS_MODE && !timeTaggerNG) {
        printf("Cannot use continuous mode with TimeTagger 1G/2G: %s\n",
               err_message);
        timetagger4_close(device);
        return nullptr;
    }
    return device;
}

int configure_timetagger(timetagger4_device* device) {
    // prepare configuration
    timetagger4_static_info static_info;
    timetagger4_get_static_info(device, &static_info);
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
        config.channel[i].start = 0; // range begins right after start pulse
        if (!USE_CONTINUOUS_MODE) {
            config.channel[i].stop = 30000; // recording window stops
                                            // after ~15 us
        }
        else {
            config.channel[i].stop = 0x7fffffff; // trigger is independent
                                                 // of stops
                                                 // set to maximal value
        }

        // measure only rising edge for tiger (positive) pulse or falling
        // for user (negative) pulse
        config.trigger[TIMETAGGER4_TRIGGER_A + i].falling = 
            USE_TIGER_STOPS ? 0 : 1;
        config.trigger[TIMETAGGER4_TRIGGER_A + i].rising = 
            USE_TIGER_STOPS ? 1 : 0;
    }

    // generate an internal 25 kHz trigger, used for tiger and continuous mode
    config.auto_trigger_period = 
        (int)(static_info.auto_trigger_ref_clock / 25000);
    config.auto_trigger_random_exponent = 0;

    // setup TiGeR
    // sending a signal to the LEMO outputs (and to the TDC on the same channel)
    // requires proper 50 Ohm termination on the LEMO output to work reliably

    // width of the 12ns pulse in the auto_trigger clock periods
    int pulse_width = (int) (12e-9 * static_info.auto_trigger_ref_clock);

    if (!USE_CONTINUOUS_MODE) {
        // use 200 kHz auto trigger to generate

        // generate above configured auto trigger to generate a 
        // signal with 12 ns pulse width on LEMO output Start
        config.tiger_block[0].enable = USE_TIGER_START ? 1 : 0;
        config.tiger_block[0].start = 0;
        config.tiger_block[0].stop = config.tiger_block[0].start + pulse_width;
        config.tiger_block[0].negate = 0;
        config.tiger_block[0].retrigger = 0;
        config.tiger_block[0].extend = 0;
        config.tiger_block[0].enable_lemo_output = 1;
        config.tiger_block[0].sources = TIMETAGGER4_TRIGGER_SOURCE_AUTO;
        // if TiGeR is used for triggering with positive pulses
        if (USE_TIGER_START)
            config.dc_offset[0] = TIMETAGGER4_DC_OFFSET_P_LVCMOS_18;
        else // user input expect NIM signal
            config.dc_offset[0] = TIMETAGGER4_DC_OFFSET_N_NIM;

        // start group on falling edges on the start channel 0
        config.trigger[TIMETAGGER4_TRIGGER_S].falling = USE_TIGER_START ? 0 : 1;
        config.trigger[TIMETAGGER4_TRIGGER_S].rising = USE_TIGER_START ? 1 : 0;
    } else {
        // Auto trigger is used as a start signal
        config.tdc_mode = TIMETAGGER4_TDC_MODE_CONTINUOUS;
    }

    for (int i = 1; i < TDC4_TIGER_COUNT; i++) {
        config.tiger_block[i].enable = USE_TIGER_STOPS ? 1 : 0;
        config.tiger_block[i].start = i * 100;
        config.tiger_block[i].stop = config.tiger_block[i].start + pulse_width;
        config.tiger_block[i].negate = 0;
        config.tiger_block[i].retrigger = 0;
        config.tiger_block[i].extend = 0;
        config.tiger_block[i].enable_lemo_output = USE_TIGER_STOPS ? 1 : 0;
        config.tiger_block[i].sources = TIMETAGGER4_TRIGGER_SOURCE_AUTO;

        if (USE_TIGER_STOPS)
            config.dc_offset[i] = TIMETAGGER4_DC_OFFSET_P_LVCMOS_18;
        else // user input expect NIM signal
            config.dc_offset[i] = TIMETAGGER4_DC_OFFSET_N_NIM;  

        // this is not related to the tigers, but uses the same indexing (0 is start)
        // optionally increase input delay by 10 * 200 ps for each channel on new TT  
        // config.delay_config[i].delay = i * 10;
    }

    // write configuration to board
    return timetagger4_configure(device, &config);
}

void print_device_information(timetagger4_device* device,
                              timetagger4_static_info* si,
                              timetagger4_param_info* pi) {
    // print board information
    printf("Board Serial        : %d.%d\n",
           si->board_serial >> 24, si->board_serial & 0xffffff);
    printf("Board Configuration : %s\n",
           timetagger4_get_device_name(device));
    printf("Board Revision      : %d\n",
           si->board_revision);
    printf("Firmware Revision   : %d.%d\n",
           si->firmware_revision, si->subversion_revision);
    printf("Driver Revision     : %d.%d.%d\n",
           ((si->driver_revision >> 16) & 255),
           ((si->driver_revision >> 8) & 255),
           (si->driver_revision & 255));
    printf("Driver SVN Revision : %d\n",
           si->driver_build_revision);
    printf("\nTDC binsize         : %0.2f ps\n",
           pi->binsize);
}

double last_abs_ts_on_a = 0;
int64_t last_group_abs_time = 0;

int64_t processPacket(volatile crono_packet* p,
                      bool print,
                      timetagger4_static_info* si,
                      timetagger4_param_info* pi){
    // do something with the data, e.g. calculate current rate
    int64_t group_abs_time = p->timestamp;
    if (!USE_CONTINUOUS_MODE) {
        // group timestamp increments at binsize, but we see only a fraction of
        // the packets (every update_count)
        double rate = 1e12 / (
                (double)(group_abs_time - last_group_abs_time)
                * pi->packet_binsize
            );
        if (print && last_group_abs_time > 0) {
            printf("\r%.6f kHz", rate / 1000.0);
            // ...or print hits (not a good idea at high data rates,
            printf("Card %d - flags %d - length %d - type %d - TS %llu\n",
                   p->card, p->flags, p->length, p->type, p->timestamp);
        }
        last_group_abs_time = group_abs_time;
    }

    int hit_count = 2 * (p->length);
    // Two hits fit into every 64 bit word. The second in the last word might
    // be empty
    // This flag tells us, whether the number of hits in the packet is odd
    if ((p->flags & TIMETAGGER4_PACKET_FLAG_ODD_HITS) != 0)
        hit_count -= 1;

    uint32_t* packet_data = (uint32_t*)(p->data);
    uint32_t rollover_count = 0;
    uint64_t rollover_period_bins = si->rollover_period;
    for (int i = 0; i < hit_count; i++)
    {
        uint32_t hit = packet_data[i];
        uint32_t channel = hit & 0xf;
        // extract hit flags
        uint32_t flags = hit >> 4 & 0xf;


        if ((flags & TIMETAGGER4_HIT_FLAG_TIME_OVERFLOW) != 0) {
            // this is a overflow of the 23/24 bit counter)
            rollover_count++;
        }
        else {
            // extract channel number (A-D)
            char channel_letter = 65 + channel;

            // extract hit timestamp
            uint32_t ts_offset = hit >> 8 & 0xffffff;

            // Convert timestamp to ns, this is relative to the start of
            // the group
            double ts_offset_ns = 
                (ts_offset + rollover_count * rollover_period_bins)
                * pi->binsize / 1000.0;

            if (USE_CONTINUOUS_MODE) {
                if (channel == 0) {
                    // compute the absolute time by adding the group time in ns
                    double abs_ts_on_a =
                        (group_abs_time * pi->packet_binsize) / 1000
                        + ts_offset_ns;
                    double diff = abs_ts_on_a - last_abs_ts_on_a;
                    if (last_abs_ts_on_a > 0 && print) {
                        printf("Time difference between hits on A  %.1f ns\n",
                               diff);
                    }
                    last_abs_ts_on_a = abs_ts_on_a;
                }
            }
            else {
                if (print)
                    printf("Hit  on channel %c - flags %d - offset %u (raw) / %.1f ns\n",
                           channel_letter, flags, ts_offset, ts_offset_ns);
            }
        }
    }
    return group_abs_time;
}

int main(int argc, char* argv[]) {
    printf("cronologic timetagger4_user_guide_example using driver: %s\n",
           timetagger4_get_driver_revision_str());
    timetagger4_device* device = initialize_timetagger(8 * 1024 * 1024, 0, 0);
    if (device == nullptr) {
        exit(1);
    }
    int status = configure_timetagger(device);
    if (status != CRONO_OK) {
        printf("Could not configure TimeTagger4: %s",
               timetagger4_get_last_error_message(device));
        timetagger4_close(device);
        return status;
    }
    timetagger4_static_info static_info;
    timetagger4_get_static_info(device, &static_info);

    timetagger4_param_info parinfo;
    timetagger4_get_param_info(device, &parinfo);

    print_device_information(device, &static_info, &parinfo);

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
        printf("Could not start capturing %s",
               timetagger4_get_last_error_message(device));
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

    int64_t group_abs_time = 0;
    int64_t group_abs_time_old = 0;
    int update_count = 100;

    printf("Reading packets:\n");
    bool no_data_printed = false;
    // read 10000 packets
    while (packet_count < 10000)
    {
        // get pointers to acquired packets
        status = timetagger4_read(device, &read_config, &read_data);
        if (status != CRONO_OK) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            // to avoid a lot of lines with no data
            if (!no_data_printed) {
                printf(" - No data! -\n");
                no_data_printed = true;
            }
        }
        else
        {
            // iterate over all packets received with the last read
            volatile crono_packet* p = read_data.first_packet;
            while (p <= read_data.last_packet)
            {
                // printf is slow, so this demo only processes every nth packet 
                // your application would of course collect every packet
                bool print = packet_count % update_count == 0;

                processPacket( p, print, &static_info, &parinfo);
                no_data_printed = false;
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