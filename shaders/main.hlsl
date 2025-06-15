struct EntryRecord {
    uint3 dispatchSize : SV_DispatchGrid;
    uint  rowCount;
};

struct WhereRecord {
    uint index;
    uint serangan_rudal;
};

struct SelectRecord {
    uint index;
    uint serangan_rudal;
};

RWStructuredBuffer<int> SeranganRudalBuffer : register(u0);

// RWStructuredBuffer<int> InputBBuffer : register(u1);

RWStructuredBuffer<int> OutputBuffer : register(u1);

RWStructuredBuffer<int> atomicCounter : register(u2);

[Shader("node")]
[NodeIsProgramEntry]
[NodeLaunch("broadcasting")]
[NumThreads(64, 1, 1)]
[NodeMaxDispatchGrid(512, 512, 1)]
[NodeId("Entry")]
void FromNode(
    uint3 dispatchThreadId: SV_DispatchThreadID,

    DispatchNodeInputRecord<EntryRecord> inputRecord,

    [MaxRecords(64)]
    [NodeId("Where")]
    NodeOutput<WhereRecord> nodeOutput
) {
    ThreadNodeOutputRecords<WhereRecord> outputRecord = 
        nodeOutput.GetThreadNodeOutputRecords(1);

    outputRecord.Get().index          = dispatchThreadId.x;
    outputRecord.Get().serangan_rudal = SeranganRudalBuffer[dispatchThreadId.x];

    outputRecord.OutputComplete();
}

[Shader("node")]
[NodeLaunch("coalescing")]
[NumThreads(64, 1, 1)]
[NodeId("Where")]
void WhereNode(
    uint3 groupThreadId: SV_GroupThreadID,

    [MaxRecords(64)]
    GroupNodeInputRecords<WhereRecord> inputRecord,

    [MaxRecords(64)]
    [NodeId("Select")]
    NodeOutput<SelectRecord> nodeOutput
) {
    const WhereRecord record = inputRecord[groupThreadId.x];

    const bool passFilter = record.serangan_rudal > 500;

    ThreadNodeOutputRecords<SelectRecord> outputRecord = 
        nodeOutput.GetThreadNodeOutputRecords(passFilter);

    if (passFilter) {
        outputRecord.Get().index          = record.index;
        outputRecord.Get().serangan_rudal = record.serangan_rudal;
    }

    outputRecord.OutputComplete();
}

[Shader("node")]
[NodeLaunch("coalescing")]
[NumThreads(64, 1, 1)]
[NodeId("Select")]
void SelectNode(
    uint3 groupThreadId: SV_GroupThreadID,

    [MaxRecords(64)]
    GroupNodeInputRecords<SelectRecord> inputRecord
) {
    const SelectRecord record  = inputRecord[groupThreadId.x];
    int                counter = atomicCounter.IncrementCounter();

    OutputBuffer[counter] = record.serangan_rudal;
}