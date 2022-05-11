/*
 * Copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk
 *
 * SUSHI is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Affero General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option) any
 * later version.
 *
 * SUSHI is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE.  See the GNU Affero General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with SUSHI.  If not, see http://www.gnu.org/licenses/
 */

/**
 * @brief 1 band equaliser plugin example
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk,
 * Stockholm
 */

#include "gate_plugin.h"
#include "spdlog/fmt/bundled/format.h"

#include <cassert>

namespace sushi {
namespace gate_plugin {

constexpr auto DEFAULT_NAME = "sushi.testing.gate";
constexpr auto DEFAULT_LABEL = "gate";

GatePlugin::GatePlugin(HostControl host_control)
    : InternalPlugin(host_control) {
    _max_input_channels = MAX_CHANNELS_SUPPORTED;
    _max_output_channels = MAX_CHANNELS_SUPPORTED;
    _current_input_channels = 1;
    _current_output_channels = 1;
    Processor::set_name(DEFAULT_NAME);
    Processor::set_label(DEFAULT_LABEL);

    for (int idx = 0; idx < MAX_CHANNELS_SUPPORTED; idx++) {
        state[idx] = CLOSED;
        gate[idx] = 0;
        holding[idx] = 0;
    }
    _sample_rate = SAMPLE_RATE_DEFAULT;
    _threshold = register_float_parameter(
        "threshold", "Gate Threshold", "db", THRESHOLD_DEFAULT, -70.0f, 12.0f,
        new dBToLinPreProcessor(THRESHOLD_DEFAULT, 12.0));

    _attack = register_float_parameter(
        "attack", "Gate Attack time", "ms", ATTACK_DEFAULT, 0.1f, 500.0f,
        new FloatParameterPreProcessor(0.01f, 500.0f));

    _hold = register_float_parameter(
        "hold", "Gate Hold time", "ms", HOLD_DEFAULT, 5.0f, 3000.0f,
        new FloatParameterPreProcessor(5.0f, 3000.0f));

    _decay = register_float_parameter(
        "decay", "Gate Decay time", "ms", DECAY_DEFAULT, 5.0f, 4000.0f,
        new FloatParameterPreProcessor(5.0f, 4000.0f));

    _range = register_float_parameter("range", "Gate Range", "db",
                                      RANGE_DEFAULT, -90.0f, -20.0f,
                                      new dBToLinPreProcessor(-90.0f, -20.0f));
    _update_rate_parameter = register_float_parameter(
        "update_rate", "Update Rate", "/s", DEFAULT_REFRESH_RATE, 0.1, 25,
        new FloatParameterPreProcessor(0.1, DEFAULT_REFRESH_RATE));

    std::string param_name = "status_{}";
    std::string param_label = "Status gate {}";
    for (int i = 0; i < MAX_CHANNELS_SUPPORTED; ++i)
    {
        _gate_status[i] = register_bool_parameter(fmt::format(param_name, i), fmt::format(param_label, i), "",
                                                        false);
        assert (_gate_status[i]);
    }

    _threshold_id = _threshold->descriptor()->id();
    _attack_id = _attack->descriptor()->id();
    _hold_id = _hold->descriptor()->id();
    _decay_id = _decay->descriptor()->id();
    _range_id = _range->descriptor()->id();
    _update_rate_id = _update_rate_parameter->descriptor()->id();

    init_values();

    assert(_threshold);
    assert(_attack);
    assert(_hold);
    assert(_decay);
    assert(_range);
}

void inline GatePlugin::init_values() {
    threshold_value = _threshold->processed_value();
    attack_coef = 1000 / (_attack->processed_value() * _sample_rate);
    hold_samples = round(_hold->processed_value() * _sample_rate * 0.001);
    decay_coef = 1000 / (_decay->processed_value() * _sample_rate);
    range_coef = _range->domain_value() > -90 ? _range->processed_value() : 0;
}

ProcessorReturnCode GatePlugin::init(float sample_rate) {
    _sample_rate = sample_rate;
    init_values();
    _update_refresh_interval(DEFAULT_REFRESH_RATE,
                             sample_rate);
    return ProcessorReturnCode::OK;
}

void GatePlugin::configure(float sample_rate) {
    _sample_rate = sample_rate;
    init_values();
    _update_refresh_interval(_update_rate_parameter->processed_value(),
                             sample_rate);
    return;
}

void GatePlugin::set_input_channels(int channels) {
    Processor::set_input_channels(channels);
    {
        _current_output_channels = channels;
        _max_output_channels = channels;
    }
}

void GatePlugin::process_event(const RtEvent &event) {
    InternalPlugin::process_event(event);

    if (event.type() == RtEventType::FLOAT_PARAMETER_CHANGE) {
        if (event.parameter_change_event()->param_id() == _threshold_id) {
            threshold_value = _threshold->processed_value();
        } else if (event.parameter_change_event()->param_id() == _attack_id) {
            attack_coef = 1000 / (_attack->processed_value() * _sample_rate);
        } else if (event.parameter_change_event()->param_id() == _hold_id) {
            hold_samples =
                round(_hold->processed_value() * _sample_rate * 0.001);
        } else if (event.parameter_change_event()->param_id() == _decay_id) {
            decay_coef = 1000 / (_decay->processed_value() * _sample_rate);
        } else if (event.parameter_change_event()->param_id() == _range_id) {
            range_coef =
                _range->domain_value() > -90 ? _range->processed_value() : 0;
        } else if (event.parameter_change_event()->param_id() ==
                   _update_rate_id) {
            _update_refresh_interval(_update_rate_parameter->processed_value(),
                                     _sample_rate);
        }
    }
}

void GatePlugin::_update_refresh_interval(float rate, float sample_rate) {
    _refresh_interval = static_cast<int>(std::round(sample_rate / rate));
}

void GatePlugin::process_audio(const ChunkSampleBuffer &in_buffer,
                               ChunkSampleBuffer &out_buffer) {
    /* Update parameter values */

    if (!_bypassed) {
        int n_channels = std::min(
            std::min(in_buffer.channel_count(), out_buffer.channel_count()),
            MAX_CHANNELS_SUPPORTED);
        for (int channel_idx = 0; channel_idx < n_channels; channel_idx++) {
            for (int sample_idx = 0; sample_idx < AUDIO_CHUNK_SIZE;
                 sample_idx++) {
                float sample = in_buffer.channel(channel_idx)[sample_idx];
                float abs_sample = fabs(sample);

                switch (state[channel_idx]) {
                    case CLOSED:
                    case DECAY:
                        if (abs_sample >= threshold_value) {
                            state[channel_idx] = ATTACK;
                        }
                        break;
                    case ATTACK:
                        break;
                    case OPENED:
                        if (abs_sample >= threshold_value) {
                            holding[channel_idx] = hold_samples;
                        } else if (holding[channel_idx] <= 0) {
                            state[channel_idx] = DECAY;
                        } else {
                            holding[channel_idx]--;
                        }
                        break;
                    default:
                        // shouldn't happen
                        state[channel_idx] = CLOSED;
                }

                // handle attack/decay in a second pass to avoid unnecessary
                // one-sample delay
                switch (state[channel_idx]) {
                    case CLOSED:
                        out_buffer.channel(channel_idx)[sample_idx] =
                            sample * range_coef;
                        break;
                    case DECAY:
                        gate[channel_idx] -= decay_coef;
                        if (gate[channel_idx] <= 0) {
                            gate[channel_idx] = 0;
                            state[channel_idx] = CLOSED;
                        }
                        out_buffer.channel(channel_idx)[sample_idx] =
                            sample * (range_coef * (1 - gate[channel_idx]) +
                                      gate[channel_idx]);
                        break;
                    case ATTACK:
                        gate[channel_idx] += attack_coef;
                        if (gate[channel_idx] >= 1) {
                            gate[channel_idx] = 1;
                            state[channel_idx] = OPENED;
                            holding[channel_idx] = hold_samples;
                        }
                        out_buffer.channel(channel_idx)[sample_idx] =
                            sample * (range_coef * (1 - gate[channel_idx]) +
                                      gate[channel_idx]);
                        break;
                    case OPENED:
                        out_buffer.channel(channel_idx)[sample_idx] = sample;
                        break;
                }
            }
                _sample_count += AUDIO_CHUNK_SIZE;
                if (_sample_count > _refresh_interval) {
                    _sample_count -= _refresh_interval;
                    set_parameter_and_notify(_gate_status[channel_idx],
                                             state[channel_idx] == CLOSED ? false : true);
                }
        }

    } else {
        bypass_process(in_buffer, out_buffer);
    }
}

void GatePlugin::_process_updates() {}

}  // namespace gate_plugin
}  // namespace sushi