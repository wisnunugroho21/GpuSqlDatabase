struct EntryRecord {
    uint3 dispatchSize : SV_DispatchGrid;
    uint  rowCount;
};

/* struct WhereRecord {
    uint index;
    uint serangan_rudal;
}; */

RWStructuredBuffer<int> InputABuffer : register(u0);

// RWStructuredBuffer<int> InputBBuffer : register(u1);

RWStructuredBuffer<int> OutputBuffer : register(u1);

[Shader("node")]
[NodeIsProgramEntry]
[NodeLaunch("broadcasting")]
[NumThreads(1, 1, 1)]
[NodeMaxDispatchGrid(10, 10, 10)]
[NodeId("Entry")]
void FromNode(
    DispatchNodeInputRecord<EntryRecord> inputRecord
) {
    OutputBuffer[0] = InputABuffer[0];
    OutputBuffer[1] = InputABuffer[1];
    OutputBuffer[2] = InputABuffer[2];
    OutputBuffer[3] = InputABuffer[3];
    OutputBuffer[4] = InputABuffer[4];
    OutputBuffer[5] = InputABuffer[5];
    OutputBuffer[6] = InputABuffer[6];
    OutputBuffer[7] = InputABuffer[7];
    OutputBuffer[8] = InputABuffer[8];
    OutputBuffer[9] = inputRecord.Get().rowCount;
}

/* [Shader("node")]
[NodeLaunch("coalescing")]
[NumThreads(64, 1, 1)]
[NodeId("Where")]
void WhereNode(
    [MaxRecords(64)]
    GroupNodeInputRecords<WhereRecord> inputRecord
) {

} */