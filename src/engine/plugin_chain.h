/**
 * @Brief Wrapper class to hold a single input - single output chain of processing plugins
 * @copyright MIND Music Labs AB, Stockholm
 *
 *
 */

#ifndef SUSHI_PLUGIN_CHAIN_H
#define SUSHI_PLUGIN_CHAIN_H

#include <memory>
#include <cassert>

#include "plugin_interface.h"
#include "library/sample_buffer.h"
#include "library/processor.h"

#include "EASTL/vector.h"

namespace sushi {
namespace engine {

/* for now, chains have at most stereo capability */
constexpr int PLUGIN_CHAIN_MAX_CHANNELS = 2;

class PluginChain : public Processor
{
public:
    PluginChain()
    {
        _max_input_channels = PLUGIN_CHAIN_MAX_CHANNELS;
        _max_output_channels = 2;
        _current_input_channels = 0;
        _current_output_channels = 0;
    }

    ~PluginChain() = default;
    MIND_DECLARE_NON_COPYABLE(PluginChain)
    /**
     * @brief Adds a plugin to the end of the chain.
     * @param The plugin to add.
     */
    void add(Processor* processor);

    /**
     * @brief handles events sent to this processor only and not sub-processors
     */
    void process_event(BaseEvent* /*event*/) override {}

    /**
     * @brief Process the entire chain and store the result in out.
     * @param in input buffer.
     * @param out output buffer.
     */
    void process_audio(const ChunkSampleBuffer& in, ChunkSampleBuffer& out)
    {
        _bfr_1.clear();
        _bfr_1.add(in);
        for (auto &plugin : _chain)
        {
            ChunkSampleBuffer in_bfr = ChunkSampleBuffer::create_non_owning_buffer(_bfr_1, 0, plugin->input_channels());
            ChunkSampleBuffer out_bfr = ChunkSampleBuffer::create_non_owning_buffer(_bfr_2, 0, plugin->output_channels());
            plugin->process_audio(in_bfr, out_bfr);
            std::swap(_bfr_1, _bfr_2);
        }
        ChunkSampleBuffer out_bfr = ChunkSampleBuffer::create_non_owning_buffer(_bfr_2, 0, _current_output_channels);
        out.add(out_bfr);
    }

private:
    /**
     * @brief Loops through the chain of plugins and negotiatiates channel configuration.
     */
    void update_channel_config();

    eastl::vector<Processor*> _chain;
    ChunkSampleBuffer _bfr_1{PLUGIN_CHAIN_MAX_CHANNELS};
    ChunkSampleBuffer _bfr_2{PLUGIN_CHAIN_MAX_CHANNELS};
};





} // namespace engine
} // namespace sushi



#endif //SUSHI_PLUGIN_CHAIN_H
