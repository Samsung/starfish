#include "core/modules/webaudio/render/AudioGraph.h"
#include "core/modules/profiling/Profiling.h"
#include "core/modules/webaudio/render/AudioParamTimeline.h"
#include "platform/webaudio/AudioOutputDevice.h"

#include <chrono>
#include <cstdio>
#include <thread>

#include <sys/time.h>

namespace Starfish {

// AudioGraph.cpp records the device-less clock with longTickCount(), which
// lives in Profiling.cpp; the harness does not link that file.
uint64_t longTickCount()
{
    struct timeval now;
    gettimeofday(&now, nullptr);
    return static_cast<uint64_t>(now.tv_sec) * 1000000 + now.tv_usec;
}

// Exercise the real graph worker without a device, DOM, or a GC runtime.
std::unique_ptr<AudioOutputDevice> AudioOutputDevice::create(uint32_t, size_t)
{
    return std::unique_ptr<AudioOutputDevice>();
}

class StressHandler final : public AudioHandler {
public:
    StressHandler()
        : AudioHandler(1, 1, 1)
    {
    }

private:
    void process(uint64_t, size_t frames) override
    {
        output(0).bus().copyFrom(input(0).bus(), frames, true);
    }
};

} // namespace Starfish

int main()
{
    using namespace Starfish;
    for (size_t iteration = 0; iteration < 32; iteration++) {
        // Alternate the default render quantum with a renderSizeHint one.
        AudioGraph graph(48000, iteration % 2 ? 100 : 128);
        auto* source = graph.addHandler(
            std::unique_ptr<AudioHandler>(new StressHandler()));
        auto* destination = graph.addHandler(
            std::unique_ptr<AudioHandler>(new StressHandler()));
        auto* timeline = graph.createTimeline(0);
        graph.setDestination(&destination->output(0));
        if (!graph.startRealtime(1)) {
            return 1;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        for (size_t i = 0; i < 2048; i++) {
            graph.queueConnection(&source->output(0), &destination->input(0),
                                  true);
            graph.queueConnection(&source->output(0), timeline, true);
            graph.queueConnection(&source->output(0), &destination->input(0),
                                  false);
            graph.queueConnection(&source->output(0), timeline, false);
            if (i % 17 == 0) {
                AudioGraphLock lock(&graph);
                destination->input(0).configure(
                    1, AudioInputChannelMode::Explicit, true);
            }
        }
        graph.stopRealtime();
        const uint64_t stopped = graph.renderedFrames();
        if (stopped == 0) {
            return 2;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        if (graph.renderedFrames() != stopped || !graph.startRealtime(1)) {
            return 3;
        }
        graph.queueConnection(&source->output(0), &destination->input(0), true);
        // Destruction must join the worker before destroying command targets.
    }
    std::puts("GRAPH_THREAD_PASS");
    return 0;
}
