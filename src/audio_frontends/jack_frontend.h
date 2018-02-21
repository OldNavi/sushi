/**
 * @brief Realtime Jackaudio frontend
 *
 */

#ifndef SUSHI_JACK_FRONTEND_H
#define SUSHI_JACK_FRONTEND_H
#ifdef SUSHI_BUILD_WITH_JACK

#include <string>
#include <tuple>
#include <vector>

#include <jack/jack.h>

#include "base_audio_frontend.h"
#include "control_frontends/osc_frontend.h"
#include "control_frontends/alsa_midi_frontend.h"

namespace sushi {

namespace audio_frontend {

constexpr int MAX_EVENTS_PER_CHUNK = 100;

struct JackFrontendConfiguration : public BaseAudioFrontendConfiguration
{
    JackFrontendConfiguration(const std::string client_name,
                              const std::string server_name,
                              bool autoconnect_ports) :
            client_name(client_name),
            server_name(server_name),
            autoconnect_ports(autoconnect_ports)
    {}

    virtual ~JackFrontendConfiguration()
    {}

    std::string client_name;
    std::string server_name;
    bool autoconnect_ports;
};

class JackFrontend : public BaseAudioFrontend
{
public:
    JackFrontend(engine::BaseEngine* engine, midi_dispatcher::MidiDispatcher* midi_dispatcher) :
            BaseAudioFrontend(engine, midi_dispatcher)
    {}

    virtual ~JackFrontend()
    {
        cleanup();
    }

    /**
     * @brief The realtime process callback given to jack and which will be
     *        called for every processing chunk.
     * @param nframes Number of frames in this processing chunk.
     * @param arg In this case is a pointer to the JackFrontend instance.
     * @return
     */
    static int rt_process_callback(jack_nframes_t nframes, void *arg)
    {
        return static_cast<JackFrontend*>(arg)->internal_process_callback(nframes);
    }

    /**
     * @brief Callback for sample rate changes
     * @param nframes New samplerate in Samples per second
     * @param arg Pointer to the JackFrontend instance.
     * @return
     */
    static int samplerate_callback(jack_nframes_t nframes, void *arg)
    {
        return static_cast<JackFrontend*>(arg)->internal_samplerate_callback(nframes);
    }

    /**
     * @brief Initialize the frontend and setup Jack client.
     * @param config Configuration struct
     * @return OK on successful initialization, error otherwise.
     */
    AudioFrontendStatus init(BaseAudioFrontendConfiguration* config) override;

    /**
     * @brief Call to clean up resources and release ports
     */
    void cleanup() override;

    /**
     * @brief Activate the realtime frontend, currently blocking.
     */
    void run() override;

private:
    /* Set up the jack client and associated ports */
    AudioFrontendStatus setup_client(const std::string client_name, const std::string server_name);
    AudioFrontendStatus setup_sample_rate();
    AudioFrontendStatus setup_ports();
    /* Call after activation to connect the frontend ports to system ports */
    AudioFrontendStatus connect_ports();

    /* Internal process callback function */
    int internal_process_callback(jack_nframes_t nframes);
    int internal_samplerate_callback(jack_nframes_t nframes);

    void process_events();
    void process_midi(jack_nframes_t start_frame, jack_nframes_t frame_count);
    void process_audio(jack_nframes_t start_frame, jack_nframes_t frame_count);

    std::array<jack_port_t*, MAX_FRONTEND_CHANNELS> _output_ports;
    std::array<jack_port_t*, MAX_FRONTEND_CHANNELS> _input_ports;
    jack_port_t* _midi_port;
    jack_client_t* _client{nullptr};
    jack_nframes_t _sample_rate;
    bool _autoconnect_ports{false};

    SampleBuffer<AUDIO_CHUNK_SIZE> _in_buffer{MAX_FRONTEND_CHANNELS};
    SampleBuffer<AUDIO_CHUNK_SIZE> _out_buffer{MAX_FRONTEND_CHANNELS};

    RtEventFifo _event_queue;

    std::unique_ptr<control_frontend::OSCFrontend> _osc_control;
    std::unique_ptr<midi_frontend::BaseMidiFrontend> _midi_frontend;
};

}; // end namespace jack_frontend
}; // end namespace sushi

#endif //SUSHI_BUILD_WITH_JACK
#ifndef SUSHI_BUILD_WITH_JACK
/* If Jack is disabled in the build config, the jack frontend is replaced with
   this dummy frontend whose only purpose is to assert if you try to use it */

#include "base_audio_frontend.h"
#include "engine/midi_dispatcher.h"
namespace sushi {
namespace audio_frontend {
struct JackFrontendConfiguration : public BaseAudioFrontendConfiguration
{
    JackFrontendConfiguration(const std::string,
                              const std::string,
                              bool ) {}
};

class JackFrontend : public BaseAudioFrontend
{
public:
    JackFrontend(engine::BaseEngine* engine,
            midi_dispatcher::MidiDispatcher* midi_dispatcher);
    AudioFrontendStatus init(BaseAudioFrontendConfiguration*) override
    {return AudioFrontendStatus::OK;}
    void cleanup() override {}
    void run() override {}
};
}; // end namespace jack_frontend
}; // end namespace sushi
#endif

#endif //SUSHI_JACK_FRONTEND_H
