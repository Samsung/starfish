// Records channel 0 of what |source| sends, every render quantum in order,
// into |blocks| (Float32Arrays), and resolves once source -> recorder ->
// destination is connected. An AudioWorklet recorder posts each quantum from
// the rendering loop, so a busy main thread delays the samples but loses
// none. Without AudioWorklet a ScriptProcessorNode records instead; its two
// buffers lose a block when the main thread falls a block behind.
function recordPCM(context, source, blocks) {
    if (typeof AudioWorkletNode !== "function" || !context.audioWorklet) {
        const processor = context.createScriptProcessor(2048, 1, 1);
        processor.onaudioprocess = function (event) {
            blocks.push(new Float32Array(event.inputBuffer.getChannelData(0)));
        };
        source.connect(processor).connect(context.destination);
        return Promise.resolve();
    }
    // An inactive input has no channels: it is recorded as silence, as a
    // ScriptProcessorNode records it. A data: module loads from a file://
    // page in browsers too; a blob: one is refused there.
    const recorder = `
        registerProcessor("pcm-recorder", class extends AudioWorkletProcessor {
            process(inputs, outputs) {
                const input = inputs[0];
                this.port.postMessage(input.length ? input[0]
                                      : new Float32Array(outputs[0][0].length));
                return true;
            }
        });`;
    return context.audioWorklet.addModule(
        "data:application/javascript," + encodeURIComponent(recorder)
    ).then(function () {
        const node = new AudioWorkletNode(context, "pcm-recorder", {
            channelCount: 1,
            channelCountMode: "explicit",
            outputChannelCount: [1],
        });
        node.port.onmessage = function (event) {
            blocks.push(event.data);
        };
        source.connect(node).connect(context.destination);
    });
}
