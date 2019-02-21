#include <thread>

#include "logging.h"

#include "library/midi_encoder.h"
#include "control_frontends/base_control_frontend.h"

namespace sushi {
namespace control_frontend {

MIND_GET_LOGGER;

constexpr int STOP_RETRIES = 200;
constexpr auto RETRY_INTERVAL = std::chrono::milliseconds(2);

void BaseControlFrontend::send_parameter_change_event(ObjectId processor,
                                                      ObjectId parameter,
                                                      float value)
{
    Time timestamp = IMMEDIATE_PROCESS;
    auto e = new ParameterChangeEvent(ParameterChangeEvent::Subtype::FLOAT_PARAMETER_CHANGE,
                                      processor, parameter, value, timestamp);
    _event_dispatcher->post_event(e);
}

void BaseControlFrontend::send_string_parameter_change_event(ObjectId processor,
                                                             ObjectId parameter,
                                                             const std::string& value)
{
    Time timestamp = IMMEDIATE_PROCESS;
    auto e = new StringPropertyChangeEvent(processor, parameter, value, timestamp);
    _event_dispatcher->post_event(e);}


void BaseControlFrontend::send_keyboard_event(ObjectId processor,
                                              KeyboardEvent::Subtype type,
                                              int channel,
                                              int note,
                                              float velocity)
{
    Time timestamp = IMMEDIATE_PROCESS;
    auto e = new KeyboardEvent(type, processor, channel, note, velocity, timestamp);
    _event_dispatcher->post_event(e);
}

void BaseControlFrontend::send_note_on_event(ObjectId processor, int channel, int note, float velocity)
{
    send_keyboard_event(processor, KeyboardEvent::Subtype::NOTE_ON, channel, note, velocity);
}

void BaseControlFrontend::send_note_off_event(ObjectId processor, int channel, int note, float velocity)
{
    send_keyboard_event(processor, KeyboardEvent::Subtype::NOTE_OFF, channel, note, velocity);
}

void BaseControlFrontend::send_program_change_event(ObjectId processor, int channel, int program)
{
    MidiDataByte midi_msg = midi::encode_program_change(channel, program);
    Time timestamp = IMMEDIATE_PROCESS;
    auto e = new KeyboardEvent(KeyboardEvent::Subtype::WRAPPED_MIDI, processor, midi_msg, timestamp);
    _event_dispatcher->post_event(e);
}

void BaseControlFrontend::send_add_track_event(const std::string &name, int channels)
{
    Time timestamp = IMMEDIATE_PROCESS;
    auto e = new AddTrackEvent(name, channels, timestamp);
    send_with_callback(e);
}

void BaseControlFrontend::send_remove_track_event(const std::string &name)
{
    Time timestamp = IMMEDIATE_PROCESS;
    auto e = new RemoveTrackEvent(name, timestamp);
    send_with_callback(e);
}

void BaseControlFrontend::send_add_processor_event(const std::string &track, const std::string &uid,
                                                   const std::string &name, const std::string &file,
                                                   AddProcessorEvent::ProcessorType type)
{
    Time timestamp = IMMEDIATE_PROCESS;
    auto e = new AddProcessorEvent(track, uid, name, file, type, timestamp);
    send_with_callback(e);
}

void BaseControlFrontend::send_remove_processor_event(const std::string &track, const std::string &name)
{
    Time timestamp = IMMEDIATE_PROCESS;
    auto e = new RemoveProcessorEvent(name, track, timestamp);
    send_with_callback(e);
}

void BaseControlFrontend::send_set_tempo_event(float tempo)
{
    auto e = new SetEngineTempoEvent(tempo, IMMEDIATE_PROCESS);
    _event_dispatcher->post_event(e);
}

void BaseControlFrontend::send_set_time_signature_event(TimeSignature signature)
{
    auto e = new SetEngineTimeSignatureEvent(signature, IMMEDIATE_PROCESS);
    _event_dispatcher->post_event(e);
}

void BaseControlFrontend::send_set_playing_mode_event(PlayingMode mode)
{
    auto e = new SetEnginePlayingModeStateEvent(mode, IMMEDIATE_PROCESS);
    _event_dispatcher->post_event(e);
}

void BaseControlFrontend::send_set_sync_mode_event(SyncMode mode)
{
    auto e = new SetEngineSyncModeEvent(mode, IMMEDIATE_PROCESS);
    _event_dispatcher->post_event(e);
}

void BaseControlFrontend::send_with_callback(Event* event)
{
    event->set_completion_cb(BaseControlFrontend::completion_callback, this);
    _event_dispatcher->post_event(event);
}

} // end namespace control_frontend
} // end namespace sushi
