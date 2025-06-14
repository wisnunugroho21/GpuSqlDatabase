RWBuffer<int> InputABuffer : register(u0);

RWBuffer<int> InputBBuffer : register(u1);

RWBuffer<int> OutputBuffer : register(u2);

[Shader("node")]
[NodeIsProgramEntry]
[NodeLaunch("broadcasting")]
[NumThreads(1, 1, 1)]
[NodeDispatchGrid(1, 1, 1)]
[NodeId("Entry")]
void EntryNode() {
    OutputBuffer[0] = InputABuffer[0] + InputBBuffer[0];
    OutputBuffer[1] = InputABuffer[1] + InputBBuffer[1];
    OutputBuffer[2] = InputABuffer[2] + InputBBuffer[2];
    OutputBuffer[3] = InputABuffer[3] + InputBBuffer[3];
    OutputBuffer[4] = InputABuffer[4] + InputBBuffer[4];
    OutputBuffer[5] = InputABuffer[5] + InputBBuffer[5];
    OutputBuffer[6] = InputABuffer[6] + InputBBuffer[6];
    OutputBuffer[7] = InputABuffer[7] + InputBBuffer[7];
    OutputBuffer[8] = InputABuffer[8] + InputBBuffer[8];
    OutputBuffer[9] = InputABuffer[9] + InputBBuffer[9];
}