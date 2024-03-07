/*
 *    Copyright (c) 2024 Project CHIP Authors
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */
#include <lib/support/UnitTestRegistration.h>
#include <tracing/backend.h>
#include <tracing/metric_event.h>

#include <nlunit-test.h>

#include <algorithm>
#include <string>
#include <vector>

using namespace chip;
using namespace chip::Tracing;

namespace chip {
namespace Tracing {

static bool operator==(const MetricEvent & lhs, const MetricEvent & rhs)
{
    if (&lhs == &rhs)
    {
        return true;
    }

    if (lhs.type() == rhs.type() && std::string(lhs.key()) == std::string(rhs.key()) && lhs.ValueType() == rhs.ValueType())
    {
        switch (lhs.ValueType())
        {
        case MetricEvent::Value::Type::kInt32:
            return lhs.ValueInt32() == rhs.ValueInt32();

        case MetricEvent::Value::Type::kUInt32:
            return lhs.ValueUInt32() == rhs.ValueUInt32();

        case MetricEvent::Value::Type::kChipErrorCode:
            return lhs.ValueErrorCode() == rhs.ValueErrorCode();

        case MetricEvent::Value::Type::kUndefined:
            return true;
        }
    }
    return false;
}
} // namespace Tracing
} // namespace chip

namespace {

// This keeps a log of all received trace items
class MetricEventBackend : public Backend
{
public:
    MetricEventBackend() {}
    const std::vector<MetricEvent> & GetMetricEvents() const { return mMetricEvents; }

    // Implementation
    virtual void LogMetricEvent(const MetricEvent & event) { mMetricEvents.push_back(event); }

private:
    std::vector<MetricEvent> mMetricEvents;
};

void TestBasicMetricEvent(nlTestSuite * inSuite, void * inContext)
{

    {
        MetricEvent event(MetricEvent::Type::kInstantEvent, "instant_event");
        NL_TEST_ASSERT(inSuite, event.type() == MetricEvent::Type::kInstantEvent);
        NL_TEST_ASSERT(inSuite, std::string(event.key()) == std::string("instant_event"));
        NL_TEST_ASSERT(inSuite, event.ValueType() == MetricEvent::Value::Type::kUndefined);
    }

    {
        MetricEvent event(MetricEvent::Type::kBeginEvent, "begin_event");
        NL_TEST_ASSERT(inSuite, event.type() == MetricEvent::Type::kBeginEvent);
        NL_TEST_ASSERT(inSuite, std::string(event.key()) == std::string("begin_event"));
        NL_TEST_ASSERT(inSuite, event.ValueType() == MetricEvent::Value::Type::kUndefined);
    }

    {
        MetricEvent event(MetricEvent::Type::kEndEvent, "end_event");
        NL_TEST_ASSERT(inSuite, event.type() == MetricEvent::Type::kEndEvent);
        NL_TEST_ASSERT(inSuite, std::string(event.key()) == std::string("end_event"));
        NL_TEST_ASSERT(inSuite, event.ValueType() == MetricEvent::Value::Type::kUndefined);
    }

    {
        MetricEvent event(MetricEvent::Type::kEndEvent, "end_event_with_int32_value", int32_t(42));
        NL_TEST_ASSERT(inSuite, event.type() == MetricEvent::Type::kEndEvent);
        NL_TEST_ASSERT(inSuite, std::string(event.key()) == std::string("end_event_with_int32_value"));
        NL_TEST_ASSERT(inSuite, event.ValueType() == MetricEvent::Value::Type::kInt32);
        NL_TEST_ASSERT(inSuite, event.ValueInt32() == 42);
    }

    {
        MetricEvent event(MetricEvent::Type::kEndEvent, "end_event_with_uint32_value", uint32_t(42));
        NL_TEST_ASSERT(inSuite, event.type() == MetricEvent::Type::kEndEvent);
        NL_TEST_ASSERT(inSuite, std::string(event.key()) == std::string("end_event_with_uint32_value"));
        NL_TEST_ASSERT(inSuite, event.ValueType() == MetricEvent::Value::Type::kUInt32);
        NL_TEST_ASSERT(inSuite, event.ValueUInt32() == 42u);
    }

    {
        MetricEvent event(MetricEvent::Type::kEndEvent, "end_event_with_error_value", CHIP_ERROR_BUSY);
        NL_TEST_ASSERT(inSuite, event.type() == MetricEvent::Type::kEndEvent);
        NL_TEST_ASSERT(inSuite, std::string(event.key()) == std::string("end_event_with_error_value"));
        NL_TEST_ASSERT(inSuite, event.ValueType() == MetricEvent::Value::Type::kChipErrorCode);
        NL_TEST_ASSERT(inSuite, chip::ChipError(event.ValueErrorCode()) == CHIP_ERROR_BUSY);
    }
}

void TestInstantMetricEvent(nlTestSuite * inSuite, void * inContext)
{
    MetricEventBackend backend;

    {
        ScopedRegistration scope(backend);

        MATTER_LOG_METRIC("event1");
        MATTER_LOG_METRIC("event2");
        MATTER_LOG_METRIC("event3");
    }

    std::vector<MetricEvent> expected = {
        MetricEvent(MetricEvent::Type::kInstantEvent, "event1"),
        MetricEvent(MetricEvent::Type::kInstantEvent, "event2"),
        MetricEvent(MetricEvent::Type::kInstantEvent, "event3"),
    };

    NL_TEST_ASSERT(inSuite, backend.GetMetricEvents().size() == expected.size());
    NL_TEST_ASSERT(
        inSuite, std::equal(backend.GetMetricEvents().begin(), backend.GetMetricEvents().end(), expected.begin(), expected.end()));
}

void TestBeginEndMetricEvent(nlTestSuite * inSuite, void * inContext)
{
    MetricEventBackend backend1;
    MetricEventBackend backend2;

    {
        ScopedRegistration scope1(backend1);
        {

            MATTER_LOG_METRIC_BEGIN("event1");
            MATTER_LOG_METRIC_BEGIN("event2");
            MATTER_LOG_METRIC_END("event2", 53);
            MATTER_LOG_METRIC_END("event1");
        }

        std::vector<MetricEvent> expected1 = {
            MetricEvent(MetricEvent::Type::kBeginEvent, "event1"),
            MetricEvent(MetricEvent::Type::kBeginEvent, "event2"),
            MetricEvent(MetricEvent::Type::kEndEvent, "event2", 53),
            MetricEvent(MetricEvent::Type::kEndEvent, "event1"),
        };

        NL_TEST_ASSERT(inSuite, backend1.GetMetricEvents().size() == expected1.size());
        NL_TEST_ASSERT(
            inSuite,
            std::equal(backend1.GetMetricEvents().begin(), backend1.GetMetricEvents().end(), expected1.begin(), expected1.end()));

        {
            ScopedRegistration scope2(backend2);

            MATTER_LOG_METRIC_BEGIN("event1");
            MATTER_LOG_METRIC_BEGIN("event2");
            MATTER_LOG_METRIC_BEGIN("event3");
            MATTER_LOG_METRIC_BEGIN("event4");
            MATTER_LOG_METRIC_END("event3", CHIP_ERROR_UNKNOWN_KEY_TYPE);
            MATTER_LOG_METRIC_END("event1", 91u);
            MATTER_LOG_METRIC_END("event2", 53);
            MATTER_LOG_METRIC_END("event4");
        }

        std::vector<MetricEvent> expected2 = {
            MetricEvent(MetricEvent::Type::kBeginEvent, "event1"),
            MetricEvent(MetricEvent::Type::kBeginEvent, "event2"),
            MetricEvent(MetricEvent::Type::kBeginEvent, "event3"),
            MetricEvent(MetricEvent::Type::kBeginEvent, "event4"),
            MetricEvent(MetricEvent::Type::kEndEvent, "event3", CHIP_ERROR_UNKNOWN_KEY_TYPE),
            MetricEvent(MetricEvent::Type::kEndEvent, "event1", 91u),
            MetricEvent(MetricEvent::Type::kEndEvent, "event2", 53),
            MetricEvent(MetricEvent::Type::kEndEvent, "event4"),
        };

        NL_TEST_ASSERT(inSuite, backend2.GetMetricEvents().size() == expected2.size());
        NL_TEST_ASSERT(
            inSuite,
            std::equal(backend2.GetMetricEvents().begin(), backend2.GetMetricEvents().end(), expected2.begin(), expected2.end()));
    }
}

void TestScopedMetricEvent(nlTestSuite * inSuite, void * inContext)
{
    MetricEventBackend backend1;
    MetricEventBackend backend2;
    MetricEventBackend backend3;
    chip::ChipError err1 = CHIP_NO_ERROR;
    chip::ChipError err2 = CHIP_NO_ERROR;
    chip::ChipError err3 = CHIP_NO_ERROR;
    chip::ChipError err4 = CHIP_NO_ERROR;

    {
        ScopedRegistration scope1(backend1);
        {
            MATTER_LOG_METRIC_SCOPE("event1", err1);
            err1 = CHIP_ERROR_BUSY;
            {
                ScopedRegistration scope2(backend2);
                MATTER_LOG_METRIC_SCOPE("event2", err2);
                err2 = CHIP_ERROR_BAD_REQUEST;

                {
                    ScopedRegistration scope3(backend3);
                    MATTER_LOG_METRIC_SCOPE("event3", err3);
                    err3 = CHIP_ERROR_EVENT_ID_FOUND;
                }
                {
                    MATTER_LOG_METRIC_SCOPE("event4", err4);
                    err4 = CHIP_ERROR_BUFFER_TOO_SMALL;
                }
            }
        }

        std::vector<MetricEvent> expected1 = {
            MetricEvent(MetricEvent::Type::kBeginEvent, "event1"),
            MetricEvent(MetricEvent::Type::kBeginEvent, "event2"),
            MetricEvent(MetricEvent::Type::kBeginEvent, "event3"),
            MetricEvent(MetricEvent::Type::kEndEvent, "event3", CHIP_ERROR_EVENT_ID_FOUND),
            MetricEvent(MetricEvent::Type::kBeginEvent, "event4"),
            MetricEvent(MetricEvent::Type::kEndEvent, "event4", CHIP_ERROR_BUFFER_TOO_SMALL),
            MetricEvent(MetricEvent::Type::kEndEvent, "event2", CHIP_ERROR_BAD_REQUEST),
            MetricEvent(MetricEvent::Type::kEndEvent, "event1", CHIP_ERROR_BUSY),
        };

        NL_TEST_ASSERT(inSuite, backend1.GetMetricEvents().size() == expected1.size());
        NL_TEST_ASSERT(
            inSuite,
            std::equal(backend1.GetMetricEvents().begin(), backend1.GetMetricEvents().end(), expected1.begin(), expected1.end()));

        std::vector<MetricEvent> expected2 = {
            MetricEvent(MetricEvent::Type::kBeginEvent, "event2"),
            MetricEvent(MetricEvent::Type::kBeginEvent, "event3"),
            MetricEvent(MetricEvent::Type::kEndEvent, "event3", CHIP_ERROR_EVENT_ID_FOUND),
            MetricEvent(MetricEvent::Type::kBeginEvent, "event4"),
            MetricEvent(MetricEvent::Type::kEndEvent, "event4", CHIP_ERROR_BUFFER_TOO_SMALL),
            MetricEvent(MetricEvent::Type::kEndEvent, "event2", CHIP_ERROR_BAD_REQUEST),
        };

        NL_TEST_ASSERT(inSuite, backend2.GetMetricEvents().size() == expected2.size());
        NL_TEST_ASSERT(
            inSuite,
            std::equal(backend2.GetMetricEvents().begin(), backend2.GetMetricEvents().end(), expected2.begin(), expected2.end()));

        std::vector<MetricEvent> expected3 = {
            MetricEvent(MetricEvent::Type::kBeginEvent, "event3"),
            MetricEvent(MetricEvent::Type::kEndEvent, "event3", CHIP_ERROR_EVENT_ID_FOUND),
        };

        NL_TEST_ASSERT(inSuite, backend3.GetMetricEvents().size() == expected3.size());
        NL_TEST_ASSERT(
            inSuite,
            std::equal(backend3.GetMetricEvents().begin(), backend3.GetMetricEvents().end(), expected3.begin(), expected3.end()));
    }
}

static int DoubleOf(int input)
{
    return input * 2;
}

void TestVerifyOrExitWithMetric(nlTestSuite * inSuite, void * inContext)
{
    MetricEventBackend backend;
    ScopedRegistration scope(backend);
    chip::ChipError err = CHIP_NO_ERROR;

    VerifyOrExitWithMetric("event0", DoubleOf(2) == 4, err = CHIP_ERROR_BAD_REQUEST);
    VerifyOrExitWithMetric("event1", DoubleOf(3) == 9, err = CHIP_ERROR_INCORRECT_STATE);

exit:
    std::vector<MetricEvent> expected = {
        MetricEvent(MetricEvent::Type::kInstantEvent, "event1", CHIP_ERROR_INCORRECT_STATE),
    };

    NL_TEST_ASSERT(inSuite, err == CHIP_ERROR_INCORRECT_STATE);
    NL_TEST_ASSERT(inSuite, backend.GetMetricEvents().size() == expected.size());
    NL_TEST_ASSERT(
        inSuite, std::equal(backend.GetMetricEvents().begin(), backend.GetMetricEvents().end(), expected.begin(), expected.end()));
}

void TestSuccessOrExitWithMetric(nlTestSuite * inSuite, void * inContext)
{
    MetricEventBackend backend;
    ScopedRegistration scope(backend);
    chip::ChipError err = CHIP_NO_ERROR;

    SuccessOrExitWithMetric("event1", err = CHIP_NO_ERROR);
    SuccessOrExitWithMetric("event2", err = CHIP_ERROR_BUSY);
    SuccessOrExitWithMetric("event3", err = CHIP_NO_ERROR);

exit:
    std::vector<MetricEvent> expected = {
        MetricEvent(MetricEvent::Type::kInstantEvent, "event2", CHIP_ERROR_BUSY),
    };

    NL_TEST_ASSERT(inSuite, err == CHIP_ERROR_BUSY);
    NL_TEST_ASSERT(inSuite, backend.GetMetricEvents().size() == expected.size());
    NL_TEST_ASSERT(
        inSuite, std::equal(backend.GetMetricEvents().begin(), backend.GetMetricEvents().end(), expected.begin(), expected.end()));
}

static const nlTest sMetricTests[] = {
    NL_TEST_DEF("BasicMetricEvent", TestBasicMetricEvent),               //
    NL_TEST_DEF("InstantMetricEvent", TestInstantMetricEvent),           //
    NL_TEST_DEF("BeginEndMetricEvent", TestBeginEndMetricEvent),         //
    NL_TEST_DEF("ScopedMetricEvent", TestScopedMetricEvent),             //
    NL_TEST_DEF("VerifyOrExitWithMetric", TestVerifyOrExitWithMetric),   //
    NL_TEST_DEF("SuccessOrExitWithMetric", TestSuccessOrExitWithMetric), //
    NL_TEST_SENTINEL()                                                   //
};

} // namespace

int TestMetricEvents()
{
    nlTestSuite theSuite = { "Metric event tests", &sMetricTests[0], nullptr, nullptr };

    // Run test suite against one context.
    nlTestRunner(&theSuite, nullptr);
    return nlTestRunnerStats(&theSuite);
}

CHIP_REGISTER_TEST_SUITE(TestMetricEvents)
