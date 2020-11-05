#include "gtest/gtest.h"

#undef private

#include "engine/audio_engine.h"

#include "control_frontends/base_control_frontend.h"
#include "test_utils/engine_mockup.h"
#include "test_utils/control_mockup.h"
#include "engine/controller/osc_controller.cpp"

using namespace midi;
using namespace sushi;
using namespace sushi::engine;
using namespace sushi::control_frontend;
using namespace sushi::midi_dispatcher;
using namespace sushi::engine::controller_impl;

constexpr float TEST_SAMPLE_RATE = 44100;
constexpr int OSC_TEST_SERVER_PORT = 24024;
constexpr int OSC_TEST_SEND_PORT = 24023;

class OscControllerEventTestFrontend : public ::testing::Test
{
protected:
    OscControllerEventTestFrontend() {}

    void SetUp()
    {
        _test_dispatcher = static_cast<EventDispatcherMockup*>(_test_engine.event_dispatcher());
        ASSERT_EQ(ControlFrontendStatus::OK, _osc_frontend.init());
        _osc_controller.set_osc_frontend(&_osc_frontend);
    }

    void TearDown() {}

    EngineMockup _test_engine{TEST_SAMPLE_RATE};

    sushi::ext::ControlMockup _controller;
    OscController _osc_controller{&_test_engine};
    OSCFrontend _osc_frontend{&_test_engine, &_controller, OSC_TEST_SERVER_PORT, OSC_TEST_SEND_PORT};
    EventDispatcherMockup* _test_dispatcher;
};

TEST_F(OscControllerEventTestFrontend, TestBasicPolling)
{
    auto send_port = _osc_controller.get_send_port();
    auto receive_port = _osc_controller.get_receive_port();

    ASSERT_EQ(send_port, 24023);
    ASSERT_EQ(receive_port, 24024);

    auto enabled_outputs = _osc_controller.get_enabled_parameter_outputs();

    ASSERT_EQ(enabled_outputs.size(), 0u);
}

TEST_F(OscControllerEventTestFrontend, TestEnablingAndDisablingOfOSCOutput)
{
    // The id for the mock processor is generated by a static atomic counter in BaseIdGenetator, so needs to be fetched.
    auto processor = _test_engine.processor_container()->processor("processor");
    ObjectId processor_id = processor->id();

    const auto parameter = processor->parameter_from_name("param 1");
    ObjectId parameter_id = parameter->id();

    auto event_status_enable = _osc_controller.enable_output_for_parameter(processor_id, parameter_id);

    ASSERT_EQ(ext::ControlStatus::OK, event_status_enable);
    auto execution_status1 = _test_dispatcher->execute_event(&_test_engine);
    ASSERT_EQ(execution_status1, EventStatus::HANDLED_OK);

    auto enabled_outputs1 = _osc_controller.get_enabled_parameter_outputs();

    ASSERT_EQ(enabled_outputs1.size(), 1u);

    ASSERT_EQ(enabled_outputs1[0], "/parameter/processor/param_1");

    auto event_status_disable = _osc_controller.disable_output_for_parameter(processor_id, parameter_id);

    ASSERT_EQ(ext::ControlStatus::OK, event_status_disable);
    auto execution_status2 = _test_dispatcher->execute_event(&_test_engine);
    ASSERT_EQ(execution_status2, EventStatus::HANDLED_OK);

    auto enabled_outputs2 = _osc_controller.get_enabled_parameter_outputs();

    ASSERT_EQ(enabled_outputs2.size(), 0u);
}