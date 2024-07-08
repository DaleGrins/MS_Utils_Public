// Copyright Dale Grinsell 2024. All Rights Reserved. 

#include "SimpleFadeIn.h"

#include "MetasoundStandardNodesCategories.h"

#define LOCTEXT_NAMESPACE "MetasoundStandardNodes_SimpleFadeIn"

namespace Metasound
{
	//the below stores name and tooltip information for each input/output pin - Name and then description.

	namespace SimpleFINodeNames
	{
		//Inputs
		METASOUND_PARAM(InTrigger, "Trigger Fade", "Triggers the fade to begin");
		METASOUND_PARAM(InResetFadeIn, "Reset Fade In", "Resets the envelope value to 0");
		METASOUND_PARAM(InFadeInTime, "Fade In Time", "Fade In Time");

		//Outputs
		METASOUND_PARAM(OutEnvParam, "Envelope Out", "Fade value output");
		METASOUND_PARAM(OutTriggerStartIn, "On Fade In Start", "Triggers when the fade in starts");
		METASOUND_PARAM(OutTriggerFinished, "On Fade In Finished", "Triggers when the fade in finishes");
	}

	FSimpleFIOperator::FSimpleFIOperator(const FCreateOperatorParams& InSettings,
		const FTimeReadRef& FadeInTimeIn,
		const FTriggerReadRef& InTriggerEnter,
		const FTriggerReadRef& InTriggerReset)
		: FadeInTime(FadeInTimeIn),
		TriggerFadeIn(InTriggerEnter),
		ResetFadeIn(InTriggerReset),
		TriggerStartIn(TDataWriteReferenceFactory<FTrigger>::CreateExplicitArgs(InSettings.OperatorSettings)),
		TriggerFinished(TDataWriteReferenceFactory<FTrigger>::CreateExplicitArgs(InSettings.OperatorSettings)),
		OutEnvelope(FFloatWriteRef::CreateNew(0.0f))
	{
		//SampleRate = InSettings.OperatorSettings.GetSampleRate();
		BlockRate = InSettings.OperatorSettings.GetActualBlockRate();
	};

	void FSimpleFIOperator::Execute()
	{
		//AdvanceBlock() moves the trigger forward along with the block. It only triggers when TriggerFrame() is called.
		TriggerStartIn->AdvanceBlock();
		TriggerFinished->AdvanceBlock();

		ResetFadeIn->ExecuteBlock([this](int32 StartFrame, int32 EndFrame)
			{

			},
			[this](int32 StartFrame, int32 EndFrame)
			{
				ProcessFade = false;
				OutEnvelopeValue = 0.0f;
				*OutEnvelope = OutEnvelopeValue;
				FadeCounter = 1.0f;
			});

		TriggerFadeIn->ExecuteBlock(
			[this](int32 StartFrame, int32 EndFrame)
			{
				if (ProcessFade == true)
				{
					
					OutEnvelopeValue = FMath::Clamp(FadeCounter / FadeBlockCount, 0.0f, 1.0f);
					*OutEnvelope = OutEnvelopeValue;
					FadeCounter += 1.0f;
					if(FadeCounter - 1.0f >= FadeBlockCount)
					{
						FadeCounter = 1.0f;
						TriggerFinished->TriggerFrame(EndFrame);
						ProcessFade = false;
					}
				}
			},
			[this](int32 StartFrame, int32 EndFrame)
			{
				TriggerStartIn->TriggerFrame(StartFrame);
				ProcessFade = true;
				FadeBlockCount = BlockRate * FadeInTime->GetSeconds();
			}
		);

	}

	const FVertexInterface& FSimpleFIOperator::DeclareVertexInterface()
	{
		using namespace SimpleFINodeNames;

		static const FVertexInterface Interface(
			FInputVertexInterface(
				TInputDataVertex<FTrigger>(METASOUND_GET_PARAM_NAME_AND_METADATA(InTrigger)),
				TInputDataVertex<FTrigger>(METASOUND_GET_PARAM_NAME_AND_METADATA(InResetFadeIn)),
				TInputDataVertex<FTime>(METASOUND_GET_PARAM_NAME_AND_METADATA(InFadeInTime), 1.0f)
			),
			FOutputVertexInterface(
				TOutputDataVertex<FTrigger>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutTriggerStartIn)),
				TOutputDataVertex<FTrigger>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutTriggerFinished)),
				TOutputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutEnvParam))
			)
		);

		return Interface;
	};

	const FNodeClassMetadata& FSimpleFIOperator::GetNodeInfo()
	{
		auto CreateNodeClassMetadata = []() -> FNodeClassMetadata
			{
				FVertexInterface NodeInterface = DeclareVertexInterface();

				FNodeClassMetadata Metadata
				{
						{ TEXT("UE"), TEXT("SimpleFadeIn"), TEXT("Audio") },
						1, // Major Version
						0, // Minor Version
						METASOUND_LOCTEXT("SimpleFadeInDisplayName", "Simple Fade In"),
						METASOUND_LOCTEXT("SimpleFadeInNodeDesc", "Fades a float value from 0 to 1 over a given time"),
						PluginAuthor,
						PluginNodeMissingPrompt,
						NodeInterface,
						{ NodeCategories::Envelopes },
						{ },
						FNodeDisplayStyle{}
				};

				return Metadata;
			};

		static const FNodeClassMetadata Metadata = CreateNodeClassMetadata();
		return Metadata;
	};

	void FSimpleFIOperator::BindInputs(FInputVertexInterfaceData& InOutVertexData)
	{
		using namespace SimpleFINodeNames;
		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InTrigger), TriggerFadeIn);
		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InResetFadeIn), ResetFadeIn);
		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InFadeInTime), FadeInTime);
	}

	void FSimpleFIOperator::BindOutputs(FOutputVertexInterfaceData& InOutVertexData)
	{
		using namespace SimpleFINodeNames;
		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(OutTriggerStartIn), TriggerStartIn);
		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(OutTriggerFinished), TriggerFinished);
		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(OutEnvParam), OutEnvelope);
	}

	TUniquePtr<IOperator> FSimpleFIOperator::CreateOperator(const FCreateOperatorParams& InParams, FBuildErrorArray& OutErrors)
	{
		using namespace SimpleFINodeNames;

		const Metasound::FDataReferenceCollection& InputCollection = InParams.InputDataReferences;
		const Metasound::FInputVertexInterface& InputInterface = DeclareVertexInterface().GetInputInterface();

		FTimeReadRef TimeInputA = InputCollection.GetDataReadReferenceOrConstructWithVertexDefault<FTime>(InputInterface, METASOUND_GET_PARAM_NAME(InFadeInTime), InParams.OperatorSettings);
		FTriggerReadRef TriggerIn = InputCollection.GetDataReadReferenceOrConstruct<FTrigger>(METASOUND_GET_PARAM_NAME(InTrigger), InParams.OperatorSettings);
		FTriggerReadRef TriggerResetIn = InputCollection.GetDataReadReferenceOrConstruct<FTrigger>(METASOUND_GET_PARAM_NAME(InResetFadeIn), InParams.OperatorSettings);

		//this class is FSimpleFIOperator, which inherits from TExecutableOperator, which inherits from IOperator. IOperator type is returned
		return MakeUnique<FSimpleFIOperator>(InParams, TimeInputA, TriggerIn, TriggerResetIn);
	}

	// Register node
	METASOUND_REGISTER_NODE(FSimpleFINode);            
}

#undef LOCTEXT_NAMESPACE
