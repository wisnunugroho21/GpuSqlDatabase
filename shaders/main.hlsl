struct WhereRecord {
    uint index;
};

RWStructuredBuffer<int> InputABuffer : register(u0);

RWStructuredBuffer<int> InputBBuffer : register(u1);

RWStructuredBuffer<int> OutputBuffer : register(u2);

// Constants provided by Work Graph Playground Application.
cbuffer Constants : register(b0)
{
    uint rowCount;
};

[Shader("node")]
[NodeIsProgramEntry]
[NodeLaunch("broadcasting")]
[NumThreads(1, 1, 1)]
[NodeDispatchGrid(1, 1, 1)]
[NodeId("Entry")]
void FromNode() {
    OutputBuffer[0] = InputABuffer[0] + InputBBuffer[0];
    OutputBuffer[1] = InputABuffer[1] + InputBBuffer[1];
    OutputBuffer[2] = InputABuffer[2] + InputBBuffer[2];
    OutputBuffer[3] = InputABuffer[3] + InputBBuffer[3];
    OutputBuffer[4] = InputABuffer[4] + InputBBuffer[4];
    OutputBuffer[5] = InputABuffer[5] + InputBBuffer[5];
    OutputBuffer[6] = InputABuffer[6] + InputBBuffer[6];
    OutputBuffer[7] = InputABuffer[7] + InputBBuffer[7];
    OutputBuffer[8] = InputABuffer[8] + InputBBuffer[8];
    OutputBuffer[9] = rowCount;
}

[Shader("node")]
[NodeLaunch("coalescing")]
[NumThreads(64, 1, 1)]
[NodeId("Where")]
void WhereNode(
    [MaxRecords(64)]
    GroupNodeInputRecords<WhereRecord> inputRecord
) {

}