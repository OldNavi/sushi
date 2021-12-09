#include <tuple>

#include "gtest/gtest.h"

#include "engine/transport.h"

#define private public
#define protected public
#include "library/internal_plugin.cpp"
#undef private
#undef protected

#include "test_utils/host_control_mockup.h"
#include "test_utils/test_utils.h"


using namespace sushi;

class TestPlugin : public InternalPlugin
{
public:
    TestPlugin(HostControl host_control) : InternalPlugin(host_control)
    {
        set_name("test_plugin");
    }

    void process_audio(const ChunkSampleBuffer &in_buffer, ChunkSampleBuffer &out_buffer) override
    {
        out_buffer = in_buffer;
    }
};


class InternalPluginTest : public ::testing::Test
{
protected:
    InternalPluginTest()
    {
    }
    void SetUp()
    {
        _module_under_test = new TestPlugin(_host_control.make_host_control_mockup());
        _module_under_test->set_event_output(&_host_control._event_output);
    }

    void TearDown()
    {
        delete(_module_under_test);
    }
    HostControlMockup _host_control;
    InternalPlugin* _module_under_test;
};


TEST_F(InternalPluginTest, TestInstanciation)
{
    EXPECT_EQ("test_plugin", _module_under_test->name());
}

TEST_F(InternalPluginTest, TestParameterRegistration)
{
    EXPECT_TRUE(_module_under_test->register_bool_parameter("bool", "Bool", "bool", false));
    EXPECT_TRUE(_module_under_test->register_property("string", "String", "default"));
    EXPECT_TRUE(_module_under_test->register_int_parameter("int", "Int", "numbers",
                                                           3, 0, 10, new IntParameterPreProcessor(0, 10)));

    EXPECT_TRUE(_module_under_test->register_float_parameter("float", "Float", "fl",
                                                             5.0f, 0.0f, 10.0f, new FloatParameterPreProcessor(0.0, 10.0)));

    // Verify all parameter/properties were registered and their order match
    auto parameter_list = _module_under_test->all_parameters();
    EXPECT_EQ(4u, parameter_list.size());

    EXPECT_EQ(4u, _module_under_test->_parameter_values.size());
    IntParameterValue* value = nullptr;
    ASSERT_NO_FATAL_FAILURE(value = _module_under_test->_parameter_values[2].int_parameter_value());
    EXPECT_EQ(3, value->processed_value());
}

TEST_F(InternalPluginTest, TestDuplicateParameterNames)
{
    auto test_param = _module_under_test->register_int_parameter("param_2", "Param 2", "", 1, 0, 10,
                                                                 new IntParameterPreProcessor(0, 10));
    EXPECT_TRUE(test_param);
    //  Register another parameter with the same name and assert that we get a null pointer back
    auto test_param_2 = _module_under_test->register_bool_parameter("param_2", "Param 2", "", false);
    EXPECT_FALSE(test_param_2);
}

TEST_F(InternalPluginTest, TestBoolParameterHandling)
{
    BoolParameterValue* value = _module_under_test->register_bool_parameter("param_1", "Param 1", "", false);
    EXPECT_TRUE(value);

    // Access the parameter through its id, verify type and that you can set its value.
    EXPECT_EQ(ParameterType::BOOL, _module_under_test->parameter_from_name("param_1")->type());
    RtEvent event = RtEvent::make_parameter_change_event(0, 0, 0, 6.0f);
    _module_under_test->process_event(event);
    EXPECT_TRUE(value->processed_value());
    // Access the parameter from the external interface
    auto [status, ext_value] = _module_under_test->parameter_value(value->descriptor()->id());
    EXPECT_EQ(ProcessorReturnCode::OK, status);
    EXPECT_FLOAT_EQ(1.0f, ext_value);
    auto [err_status, unused_value] = _module_under_test->parameter_value(45);
    EXPECT_EQ(ProcessorReturnCode::PARAMETER_NOT_FOUND, err_status);

    DECLARE_UNUSED(unused_value);
}

TEST_F(InternalPluginTest, TestIntParameterHandling)
{
    auto value = _module_under_test->register_int_parameter("param_1", "Param 1", "",
                                                            0, 0, 10,
                                                            new IntParameterPreProcessor(0, 10));
    EXPECT_TRUE(value);

    // Access the parameter through its id, verify type and that you can set its value.
    EXPECT_EQ(ParameterType::INT, _module_under_test->parameter_from_name("param_1")->type());

    RtEvent event = RtEvent::make_parameter_change_event(0, 0, 0, 0.6f);

    _module_under_test->process_event(event);
    EXPECT_FLOAT_EQ(6.0f, value->processed_value());

    // Access the parameter from the external interface
    auto [status, ext_value] = _module_under_test->parameter_value_in_domain(value->descriptor()->id());
    EXPECT_EQ(ProcessorReturnCode::OK, status);
    EXPECT_FLOAT_EQ(6.0f, ext_value);

    auto [status_1, norm_value] = _module_under_test->parameter_value(value->descriptor()->id());
    EXPECT_EQ(ProcessorReturnCode::OK, status_1);
    EXPECT_FLOAT_EQ(0.6f, norm_value);

    auto [err_status, unused_value] = _module_under_test->parameter_value(45);
    EXPECT_EQ(ProcessorReturnCode::PARAMETER_NOT_FOUND, err_status);

    DECLARE_UNUSED(unused_value);
}

TEST_F(InternalPluginTest, TestFloatParameterHandling)
{
    auto value = _module_under_test->register_float_parameter("param_1", "Param 1", "",
                                                              1.0f, 0.0f, 10.f,
                                                              new FloatParameterPreProcessor(0.0f, 10.0f));
    EXPECT_TRUE(value);

    // Access the parameter through its id, verify type and that you can set its value.
    EXPECT_EQ(ParameterType::FLOAT, _module_under_test->parameter_from_name("param_1")->type());

    RtEvent event = RtEvent::make_parameter_change_event(0, 0, 0, 0.5f);
    _module_under_test->process_event(event);
    EXPECT_EQ(5, value->processed_value());

    // Access the parameter from the external interface
    auto [status, ext_value] = _module_under_test->parameter_value_in_domain(value->descriptor()->id());
    EXPECT_EQ(ProcessorReturnCode::OK, status);
    EXPECT_FLOAT_EQ(5.0f, ext_value);

    auto [status_1, norm_value] = _module_under_test->parameter_value(value->descriptor()->id());
    EXPECT_EQ(ProcessorReturnCode::OK, status_1);
    EXPECT_FLOAT_EQ(0.5f, norm_value);

    [[maybe_unused]] auto [err_status, unused_value] = _module_under_test->parameter_value(45);
    EXPECT_EQ(ProcessorReturnCode::PARAMETER_NOT_FOUND, err_status);

    DECLARE_UNUSED(unused_value);
}

TEST_F(InternalPluginTest, TestPropertyHandling)
{
    auto descriptor = _module_under_test->register_property("str_1", "Str_1", "test");
    ASSERT_TRUE(descriptor);

    // Access the property through its id, verify type and that you can set its value.
    auto param = _module_under_test->parameter_from_name("str_1");
    ASSERT_TRUE(param);
    EXPECT_EQ(ParameterType::STRING, param->type());

    // String properties are set directly in a non-rt thread.
    EXPECT_EQ("test", _module_under_test->property_value(param->id()).second);
    EXPECT_NE(ProcessorReturnCode::OK, _module_under_test->property_value(12345).first);

    EXPECT_EQ(ProcessorReturnCode::OK, _module_under_test->set_property_value(param->id(), "updated"));
    EXPECT_EQ("updated", _module_under_test->property_value(param->id()).second);

    EXPECT_NE(ProcessorReturnCode::OK, _module_under_test->set_property_value(12345, "no_property"));
}

TEST_F(InternalPluginTest, TestSendingPropertyToRealtime)
{
    _module_under_test->register_property("property", "Property", "default");
    _module_under_test->send_property_to_realtime(0, "test");

    // Check that an event was generated and queued
    auto event = _host_control._dummy_dispatcher.retrieve_event();
    ASSERT_TRUE(event.operator bool());
    EXPECT_TRUE(event->maps_to_rt_event());
    auto rt_event = event->to_rt_event(0);
    EXPECT_EQ(RtEventType::STRING_PROPERTY_CHANGE, rt_event.type());

    // Pass the RtEvent to the plugin an verify that a delete event was generated in response
    _module_under_test->process_event(rt_event);
    RtEvent response_event;
    ASSERT_TRUE(_host_control._event_output.pop(response_event));
    EXPECT_EQ(RtEventType::STRING_DELETE, response_event.type());

    // Delete the string manually, otherwise done by the dispatcher.
    delete reinterpret_cast<std::string*>(response_event.data_payload_event()->value().data);
}


TEST_F(InternalPluginTest, TestSendingDataToRealtime)
{
    int a = 123;
    BlobData data{.size = sizeof(a), .data = (uint8_t*) &a};
    _module_under_test->send_data_to_realtime(data, 15);

    // Check that an event was generated and queued
    auto event = _host_control._dummy_dispatcher.retrieve_event();
    ASSERT_TRUE(event.operator bool());
    EXPECT_TRUE(event->maps_to_rt_event());
    auto rt_event = event->to_rt_event(0);
    EXPECT_EQ(RtEventType::DATA_PROPERTY_CHANGE, rt_event.type());

    EXPECT_EQ(123, *rt_event.data_parameter_change_event()->value().data);
}

TEST_F(InternalPluginTest, TestStateHandling)
{
    auto parameter = _module_under_test->register_float_parameter("param_1", "Param 1", "",
                                                                  1.0f, 0.0f, 10.f,
                                                                  new FloatParameterPreProcessor(0.0f, 10.0f));
    ASSERT_TRUE(parameter);
    auto property = _module_under_test->register_property("str_1", "Str_1", "test");
    ASSERT_TRUE(property);
    auto descriptor = _module_under_test->parameter_from_name("str_1");

    ProcessorState state;
    state.set_bypass(true);
    state.add_parameter_change(parameter->descriptor()->id(), 0.25);
    state.add_property_change(descriptor->id(), "new_value");

    auto status = _module_under_test->set_state(&state, false);
    ASSERT_EQ(ProcessorReturnCode::OK, status);

    // Check that new values are set
    EXPECT_FLOAT_EQ(0.25f, _module_under_test->parameter_value(parameter->descriptor()->id()).second);
    EXPECT_EQ("new_value", _module_under_test->property_value(descriptor->id()).second);
    EXPECT_TRUE(_module_under_test->bypassed());
}