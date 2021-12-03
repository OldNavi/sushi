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
 * @brief Alsa midi frontend
 * @copyright 2017-2021 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#include <functional>

#include "rt_midi_frontend.h"
#include "library/midi_decoder.h"
#include "library/time.h"
#include "logging.h"

SUSHI_GET_LOGGER_WITH_MODULE_NAME("rtmidi");

using RtMidiFunction = std::function<void(double deltatime, std::vector<unsigned char> message, void* user_data)>;

constexpr int RTMIDI_MESSAGE_SIZE = 3;
namespace sushi {
namespace midi_frontend {

void midi_callback([[maybe_unused]] double deltatime, std::vector<unsigned char>* message, void* user_data)
{
    auto* callback_data = static_cast<RtMidiCallbackData*>(user_data);
    auto byte_count = message->size();
    if (byte_count > 0)
    {
        const uint8_t* data_buffer = static_cast<const uint8_t*>(message->data());
        Time timestamp = IMMEDIATE_PROCESS;
        callback_data->receiver->send_midi(callback_data->input_number, midi::to_midi_data_byte(data_buffer, byte_count), timestamp);

        SUSHI_LOG_DEBUG("Received midi message: [{:x} {:x} {:x} {:x}], port{}, timestamp: {}",
                         data_buffer[0], data_buffer[1], data_buffer[2], data_buffer[3], callback_data->input_number, timestamp.count());
    }
    SUSHI_LOG_WARNING_IF(byte_count < 0, "Decoder returned {}", strerror(-byte_count));
}

RtMidiFrontend::RtMidiFrontend(int inputs, int outputs, midi_receiver::MidiReceiver* dispatcher)
        : BaseMidiFrontend(dispatcher),
          _inputs(inputs),
          _outputs(outputs)
{}

RtMidiFrontend::~RtMidiFrontend()
{

}

bool RtMidiFrontend::init()
{
    // Set up inputs
    for (int i = 0; i < _inputs; i++)
    {
        try
        {
            auto& callback_data = _input_midi_ports.emplace_back();
            callback_data.input_number = i;
            callback_data.receiver = _receiver;
        }
        catch (RtMidiError& error)
        {
            SUSHI_LOG_WARNING("Failed to create midi input port for input {}: {}", i, error.getMessage());
            return false;
        }
    }

    // Set up outputs
    for (int i = 0; i < _outputs; i++)
    {
        try
        {
            _output_midi_ports.emplace_back();
        }
        catch (RtMidiError& error)
        {
            SUSHI_LOG_WARNING("Failed to create midi output port for output {}: {}", i, error.getMessage());
            return false;
        }
    }
    return true;
}

void RtMidiFrontend::run()
{
    for (auto& input : _input_midi_ports)
    {
        input.midi_input.openPort(0);
        SUSHI_LOG_INFO("Midi input connected to {}", input.midi_input.getPortName(0));
        input.midi_input.setCallback(midi_callback, static_cast<void*>(&input));
    }

    for (auto& output : _output_midi_ports)
    {
        for (int i = 0; i < output.getPortCount(); i++)
        {
            SUSHI_LOG_INFO("Port {} has name {}", i, output.getPortName(i));
        }
        output.openPort(1);
        SUSHI_LOG_INFO("Midi output connected to {}", output.getPortName(1));
    }
}

void RtMidiFrontend::stop()
{
    for (auto& input : _input_midi_ports)
    {
        input.midi_input.closePort();
    }

    for (auto& output : _output_midi_ports)
    {
        output.closePort();
    }
}

void RtMidiFrontend::send_midi(int input, MidiDataByte data, Time timestamp)
{
    // Ignoring sysex for now
    std::vector<unsigned char> message(data.data(), data.data() + RTMIDI_MESSAGE_SIZE);
    _output_midi_ports[input].sendMessage(&message);
}

} // midi_frontend
} // sushi