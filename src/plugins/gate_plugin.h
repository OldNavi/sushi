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
 * @brief 1 band gate plugin example
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifndef GATE_PLUGIN_H
#define GATE_PLUGIN_H

#include "library/internal_plugin.h"
#include "dsp_library/biquad_filter.h"

#define CLOSED 1
#define ATTACK 2
#define OPENED 3
#define DECAY 4

namespace sushi {
namespace gate_plugin {

constexpr int MAX_CHANNELS_SUPPORTED = 2;

class GatePlugin : public InternalPlugin
{
public:
    GatePlugin(HostControl hostControl);

    ~GatePlugin() = default;

    virtual ProcessorReturnCode init(float sample_rate) override;

    void configure(float sample_rate) override;

    void set_input_channels(int channels) override;

    void process_audio(const ChunkSampleBuffer &in_buffer, ChunkSampleBuffer &out_buffer) override;

private:
    float _sample_rate;
    int state[MAX_CHANNELS_SUPPORTED];
    float gate[MAX_CHANNELS_SUPPORTED];
    int holding[MAX_CHANNELS_SUPPORTED];

    FloatParameterValue* _threshold;
    FloatParameterValue* _attack;
    FloatParameterValue* _hold;
    FloatParameterValue* _decay;
    FloatParameterValue* _range;

};

}// namespace gate_plugin
}// namespace sushi
#endif // GATE_PLUGIN_H