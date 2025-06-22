struct FromRecord {
    uint dispatchSize : SV_DispatchGrid;
    uint korban_rowCount;
};

struct JoinRecord {
    uint dispatchSize : SV_DispatchGrid;
    int  serangan_negaraPenyerang[64];
    int  serangan_seranganRudal[64];
};

struct OnRecord {
    int  serangan_negaraPenyerang;
    int  serangan_seranganRudal;
    int  korban_negara;
    int  korban_korbanJiwa;
};

struct WhereRecord {
    int serangan_seranganRudal;
    int korban_korbanJiwa;
};

struct SelectRecord {
    int korban_korbanJiwa;
};

RWStructuredBuffer<int> Serangan_NegaraPenyerangBuffer : register(u0);

RWStructuredBuffer<int> Serangan_SeranganRudalBuffer : register(u1);

RWStructuredBuffer<int> Korban_NegaraBuffer : register(u2);

RWStructuredBuffer<int> Korban_KorbanJiwaBuffer : register(u3);

RWStructuredBuffer<int> Output_Korban_KorbanJiwaBuffer : register(u4);

RWStructuredBuffer<int> atomicCounter : register(u5);

[Shader("node")]
[NodeIsProgramEntry]
[NodeLaunch("broadcasting")]
[NumThreads(64, 1, 1)]
[NodeMaxDispatchGrid(512, 512, 1)]
[NodeId("From")]
void FromNode(
    uint dispatchThreadId: SV_DispatchThreadID,
    uint groupThreadId: SV_GroupThreadID,

    DispatchNodeInputRecord<FromRecord> inputRecord,

    [MaxRecords(1)]
    [NodeId("Join")]
    NodeOutput<JoinRecord> nodeOutput
) {
    GroupNodeOutputRecords<JoinRecord> outputRecord = 
        nodeOutput.GetGroupNodeOutputRecords(1);

    outputRecord.Get().dispatchSize                            = inputRecord.Get().korban_rowCount;
    outputRecord.Get().serangan_negaraPenyerang[groupThreadId] = Serangan_NegaraPenyerangBuffer[dispatchThreadId];
    outputRecord.Get().serangan_seranganRudal[groupThreadId]   = Serangan_SeranganRudalBuffer[dispatchThreadId];

    outputRecord.OutputComplete();
}

[Shader("node")]
[NodeLaunch("broadcasting")]
[NumThreads(64, 1, 1)]
[NodeMaxDispatchGrid(512, 512, 1)]
[NodeId("Join")]
void JoinNode(    
    uint groupThreadId: SV_GroupThreadID,
    uint groupId: SV_GroupID,

    DispatchNodeInputRecord<JoinRecord> inputRecord,

    [MaxRecords(1)]
    [NodeId("On")]
    NodeOutput<OnRecord> nodeOutput
) {    
    ThreadNodeOutputRecords<OnRecord> outputRecord = 
        nodeOutput.GetThreadNodeOutputRecords(1);

    outputRecord.Get().serangan_negaraPenyerang = inputRecord.Get().serangan_negaraPenyerang[groupThreadId];
    outputRecord.Get().serangan_seranganRudal   = inputRecord.Get().serangan_seranganRudal[groupThreadId];
    outputRecord.Get().korban_negara            = Korban_NegaraBuffer[groupId];
    outputRecord.Get().korban_korbanJiwa        = Korban_KorbanJiwaBuffer[groupId];

    outputRecord.OutputComplete();
}

[Shader("node")]
[NodeLaunch("thread")]
[NodeId("On")]
void OnNode(
    ThreadNodeInputRecord<OnRecord> inputRecord,

    [MaxRecords(1)]
    [NodeId("Where")]
    NodeOutput<WhereRecord> nodeOutput
) {
    const bool passJoin =
        inputRecord.Get().korban_negara == inputRecord.Get().serangan_negaraPenyerang;

    ThreadNodeOutputRecords<WhereRecord> outputRecord = 
        nodeOutput.GetThreadNodeOutputRecords(passJoin);

    if (passJoin) {
        outputRecord.Get().serangan_seranganRudal = inputRecord.Get().serangan_seranganRudal;
        outputRecord.Get().korban_korbanJiwa      = inputRecord.Get().korban_korbanJiwa;
    }

    outputRecord.OutputComplete();
}

[Shader("node")]
[NodeLaunch("thread")]
[NodeId("Where")]
void WhereNode(
    ThreadNodeInputRecord<WhereRecord> inputRecord,

    [MaxRecords(1)]
    [NodeId("Select")]
    NodeOutput<SelectRecord> nodeOutput
) {
    const WhereRecord record = inputRecord.Get();

    const bool passFilter = record.serangan_seranganRudal > 500;

    ThreadNodeOutputRecords<SelectRecord> outputRecord = 
        nodeOutput.GetThreadNodeOutputRecords(passFilter);

    if (passFilter) {
        outputRecord.Get().korban_korbanJiwa = record.korban_korbanJiwa;
    }

    outputRecord.OutputComplete();
}

[Shader("node")]
[NodeLaunch("thread")]
[NodeId("Select")]
void SelectNode(
    ThreadNodeInputRecord<SelectRecord> inputRecord
) {
    const SelectRecord record  = inputRecord.Get();
    int                counter = atomicCounter.IncrementCounter();

    Output_Korban_KorbanJiwaBuffer[counter] = record.korban_korbanJiwa;
}