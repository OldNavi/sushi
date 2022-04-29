/*
 * Copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk
 *
 * SUSHI is free software: you can redistribute it and/or modify it under the terms of
 * the GNU Affero General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * SUSHI is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License along with
 * SUSHI.  If not, see http://www.gnu.org/licenses/
 */

/**
 * @brief Audio processor with event output example
 * @copyright 2017-2021 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#include "sample_delay_plugin.h"

namespace sushi {
namespace sample_delay_plugin {

constexpr auto DEFAULT_NAME = "sushi.testing.sample_delay";
constexpr auto DEFAULT_LABEL = "Sample delay";

SampleDelayPlugin::SampleDelayPlugin(HostControl host_control) : InternalPlugin(host_control),
                                                                 _write_idx_ch1(0),
                                                                 _read_idx_ch1(0),
                                                                 _write_idx_ch2(0),
                                                                 _read_idx_ch2(0)
{
    Processor::set_name(DEFAULT_NAME);
    Processor::set_label(DEFAULT_LABEL);
    _sample_delay_ch1 = register_int_parameter("sample_delay_ch1", 
                                           "Sample delay Channel 1", 
                                           "samples", 
                                           0,
                                           0,
                                           MAX_DELAY - 1);
    _sample_delay_ch2 = register_int_parameter("sample_delay_ch2", 
                                           "Sample delay Channel 2", 
                                           "samples", 
                                           0,
                                           0,
                                           MAX_DELAY - 1);
    for (int i = 0; i < DEFAULT_CHANNELS; i++)
    {
        _delaylines.push_back(std::array<float, MAX_DELAY>());
    }
}

void SampleDelayPlugin::process_audio(const ChunkSampleBuffer &in_buffer, ChunkSampleBuffer &out_buffer)
{
    // update delay value
    _read_idx_ch1 = (_write_idx_ch1 + MAX_DELAY - _sample_delay_ch1->processed_value()) % MAX_DELAY;
    _read_idx_ch2 = (_write_idx_ch2 + MAX_DELAY - _sample_delay_ch2->processed_value()) % MAX_DELAY;

    // process
    if (_bypassed == false)
    {
        int n_channels = std::min(std::min(in_buffer.channel_count(), out_buffer.channel_count()),DEFAULT_CHANNELS);
        for (int channel_idx = 0; channel_idx < n_channels; channel_idx++)
        {
            int temp_write_idx = channel_idx % 2 ? _write_idx_ch2 : _write_idx_ch1;
            int temp_read_idx =  channel_idx % 2 ? _read_idx_ch2 : _read_idx_ch1;
            for (int sample_idx = 0; sample_idx < AUDIO_CHUNK_SIZE; sample_idx++)
            {
                _delaylines[channel_idx][temp_write_idx] = in_buffer.channel(channel_idx)[sample_idx];
                out_buffer.channel(channel_idx)[sample_idx] = _delaylines[channel_idx][temp_read_idx];
                temp_write_idx++;
                temp_read_idx++;
                temp_write_idx %= MAX_DELAY;
                temp_read_idx %= MAX_DELAY;
            }
        }
        _write_idx_ch1 += AUDIO_CHUNK_SIZE;
        _write_idx_ch2 += AUDIO_CHUNK_SIZE;
        _read_idx_ch1 += AUDIO_CHUNK_SIZE;
        _read_idx_ch2 += AUDIO_CHUNK_SIZE;
        _write_idx_ch1 %= MAX_DELAY;
        _write_idx_ch2 %= MAX_DELAY;
        _read_idx_ch1 %= MAX_DELAY;
        _read_idx_ch2 %= MAX_DELAY;
    }
    else
    {
        bypass_process(in_buffer, out_buffer);
    }
}


    
} // namespace sample_delay_plugin
} // namespace sushi
