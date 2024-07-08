// Copyright Dale Grinsell 2024. All Rights Reserved. 

#include "SimpleFadeOut.h"

#include "MetasoundStandardNodesCategories.h"

#define LOCTEXT_NAMESPACE "MetasoundStandardNodes_SimpleFadeOut"

namespace Metasound
{
	//the below stores name and tooltip information for each input/output pin - Name and then description.

	namespace SimpleFONodeNames
	{
		//Inputs
		METASOUND_PARAM(InTrigger, "Trigger Fade", "Triggers the fade to begin");
		METASOUND_PARAM(InResetFadeOut, "Reset Fade Out", "Resets the envelope value to 1.0");
		METASOUND_PARAM(InFadeOutTime, "Fade Out Time", "Fade Out Time");

		//Outputs
		METASOUND_PARAM(OutEnvParam, "Envelope Out", "Fade value output");
		METASOUND_PARAM(OutTriggerStartOut, "On Fade Out Start", "Triggers when the fade out starts");
		METASOUND_PARAM(OutTriggerFinished, "On Fade Out Finished", "Triggers when the fade out finishes");
	}

	FSimpleFOOperator::FSimpleFOOperator(const FCreateOperatorParams& InSettings,
		const FTimeReadRef& FadeOutTimeIn,
		const FTriggerReadRef& InTriggerEnter,
		const FTriggerReadRef& InTriggerReset)
		: FadeOutTime(FadeOutTimeIn),
		TriggerFadeOut(InTriggerEnter),
		ResetFadeOut(InTriggerReset),
		TriggerStartOut(TDataWriteReferenceFactory<FTrigger>::CreateExplicitArgs(InSettings.OperatorSettings)),
		TriggerFinished(TDataWriteReferenceFactory<FTrigger>::CreateExplicitArgs(InSettings.OperatorSettings)),
		OutEnvelope(FFloatWriteRef::CreateNew(1.0f))
	{
		//SampleRate = InSettings.OperatorSettings.GetSampleRate();
		BlockRate = InSettings.OperatorSettings.GetActualBlockRate();
	};

	void FSimpleFOOperator::Execute()
	{
		//AdvanceBlock() moves the trigger forward along with the block. It only triggers when TriggerFrame() is called.
		TriggerStartOut->AdvanceBlock();
		TriggerFinished->AdvanceBlock();

		ResetFadeOut->ExecuteBlock([this](int32 StartFrame, int32 EndFrame)
			{

			},
			[this](int32 StartFrame, int32 EndFrame)
			{
				ProcessFade = false;
				OutEnvelopeValue = 1.0f;
				*OutEnvelope = OutEnvelopeValue;
				FadeCounter = 1.0f;
			});

		TriggerFadeOut->ExecuteBlock(
			[this](int32 StartFrame, int32 EndFrame)
			{
				if (ProcessFade == true)
				{
					
					OutEnvelopeValue = FMath::Clamp(1.0f - (FadeCounter / FadeBlockCount), 0.0f, 1.0f);
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
				TriggerStartOut->TriggerFrame(StartFrame);
				ProcessFade = true;
				FadeBlockCount = BlockRate * FadeOutTime->GetSeconds();
			}
		);

	}

	const FVertexInterface& FSimpleFOOperator::DeclareVertexInterface()
	{
		using namespace SimpleFONodeNames;

		static const FVertexInterface Interface(
			FInputVertexInterface(
				TInputDataVertex<FTrigger>(METASOUND_GET_PARAM_NAME_AND_METADATA(InTrigger)),
				TInputDataVertex<FTrigger>(METASOUND_GET_PARAM_NAME_AND_METADATA(InResetFadeOut)),
				TInputDataVertex<FTime>(METASOUND_GET_PARAM_NAME_AND_METADATA(InFadeOutTime), 1.0f)
			),
			FOutputVertexInterface(
				TOutputDataVertex<FTrigger>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutTriggerStartOut)),
				TOutputDataVertex<FTrigger>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutTriggerFinished)),
				TOutputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutEnvParam))
			)
		);

		return Interface;
	};

	const FNodeClassMetadata& FSimpleFOOperator::GetNodeInfo()
	{
		auto CreateNodeClassMetadata = []() -> FNodeClassMetadata
			{
				FVertexInterface NodeInterface = DeclareVertexInterface();

				FNodeClassMetadata Metadata
				{
						{ TEXT("UE"), TEXT("SimpleFadeOut"), TEXT("Audio") },
						1, // Major Version
						0, // Minor Version
						METASOUND_LOCTEXT("SimpleFadeOutDisplayName", "Simple Fade Out"),
						METASOUND_LOCTEXT("SimpleFadeOutNodeDesc", "Fades a float value from 1 to 0 over a given time"),
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

	void FSimpleFOOperator::BindInputs(FInputVertexInterfaceData& InOutVertexData)
	{
		using namespace SimpleFONodeNames;
		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InTrigger), TriggerFadeOut);
		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InResetFadeOut), ResetFadeOut);
		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InFadeOutTime), FadeOutTime);
	}

	void FSimpleFOOperator::BindOutputs(FOutputVertexInterfaceData& InOutVertexData)
	{
		using namespace SimpleFONodeNames;
		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(OutTriggerStartOut), TriggerStartOut);
		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(OutTriggerFinished), TriggerFinished);
		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(OutEnvParam), OutEnvelope);
	}

	TUniquePtr<IOperator> FSimpleFOOperator::CreateOperator(const FCreateOperatorParams& InParams, FBuildErrorArray& OutErrors)
	{
		using namespace SimpleFONodeNames;

		const Metasound::FDataReferenceCollection& InputCollection = InParams.InputDataReferences;
		const Metasound::FInputVertexInterface& InputInterface = DeclareVertexInterface().GetInputInterface();

		FTimeReadRef TimeInputA = InputCollection.GetDataReadReferenceOrConstructWithVertexDefault<FTime>(InputInterface, METASOUND_GET_PARAM_NAME(InFadeOutTime), InParams.OperatorSettings);
		FTriggerReadRef TriggerIn = InputCollection.GetDataReadReferenceOrConstruct<FTrigger>(METASOUND_GET_PARAM_NAME(InTrigger), InParams.OperatorSettings);
		FTriggerReadRef TriggerResetIn = InputCollection.GetDataReadReferenceOrConstruct<FTrigger>(METASOUND_GET_PARAM_NAME(InResetFadeOut), InParams.OperatorSettings);

		//this class is FSimpleFOOperator, which inherits from TExecutableOperator, which inherits from IOperator. IOperator type is returned
		return MakeUnique<FSimpleFOOperator>(InParams, TimeInputA, TriggerIn, TriggerResetIn);
	}

	// Register node
	METASOUND_REGISTER_NODE(FSimpleFONode);            
}

#undef LOCTEXT_NAMESPACE
