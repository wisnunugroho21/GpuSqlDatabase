struct EntryRecord {
    uint dispatchSize : SV_DispatchGrid;
    uint rowCount;
};

struct WhereRecord {
    uint serangan_seranganRudal;
};

struct SelectRecord {
    uint serangan_seranganRudal;
};

RWStructuredBuffer<int> Serangan_SeranganRudalBuffer : register(u0);

RWStructuredBuffer<int> Output_Serangan_SeranganRudalBuffer : register(u1);

RWStructuredBuffer<int> atomicCounter : register(u2);

[Shader("node")]
[NodeIsProgramEntry]
[NodeLaunch("broadcasting")]
[NumThreads(64, 1, 1)]
[NodeMaxDispatchGrid(512, 512, 1)]
[NodeId("Entry")]
void FromNode(
    uint dispatchThreadId: SV_DispatchThreadID,

    DispatchNodeInputRecord<EntryRecord> inputRecord,

    [MaxRecords(64)]
    [NodeId("Where")]
    NodeOutput<WhereRecord> nodeOutput
) {
    ThreadNodeOutputRecords<WhereRecord> outputRecord = 
        nodeOutput.GetThreadNodeOutputRecords(1);

    outputRecord.Get().serangan_rudal = Serangan_SeranganRudalBuffer[dispatchThreadId];

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

    const bool passFilter = record.serangan_seranganRudal > 500;

    ThreadNodeOutputRecords<SelectRecord> outputRecord = 
        nodeOutput.GetThreadNodeOutputRecords(passFilter);

    if (passFilter) {
        outputRecord.Get().serangan_seranganRudal = record.serangan_seranganRudal;
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

    Output_Serangan_SeranganRudalBuffer[counter] = record.serangan_seranganRudal;
}