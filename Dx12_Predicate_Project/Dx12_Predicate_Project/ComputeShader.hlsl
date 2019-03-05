
//
//struct Predicate
//{
//	double predVal;
//};
//
//RWStructuredBuffer<Predicate> PredicateBuffer : register(u0)
//
//[numthreads(threadBlockSize, 1, 1)]
//void CSMain(uint3 groupId : SV_GroupID, uint groupIndex : SV_GroupIndex)
//{
//	uint index = (groupId.x * threadBlockSize) + groupIndex;
//
//	// PredicateBuffer[index].predVal = 1 ? 0 
//
//
//}

#define threadBlockSize 32 //TBD

struct Predicate
{
	double predVal;
};

RWStructuredBuffer<Predicate> PredicateBuffer : register(u0);

cbuffer CB : register(b0)
{
	int index;
}

[numthreads(threadBlockSize, 1, 1)]
void CS_main(uint3 groupId : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
	uint tid = (groupId.x * threadBlockSize) + groupIndex;
	if (tid == 0)
	{
		PredicateBuffer[index].predVal = 0;
	}
}