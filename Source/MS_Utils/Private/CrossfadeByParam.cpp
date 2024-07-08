// Copyright Dale Grinsell 2024. All Rights Reserved. 

#include "CrossfadeByParam.h"

#include "DSP/FloatArrayMath.h"
#include "MetasoundStandardNodesCategories.h"

#define LOCTEXT_NAMESPACE "MetasoundStandardNodes_CrossfadeByParam"

namespace Metasound
{
	//the below stores name and tooltip information for each input/output pin - Name and then description.

	namespace ECBPNodeNames
	{
		METASOUND_PARAM(InFloatValue, "Input Value", "Input Value");
		METASOUND_PARAM(InFadeInStart, "Fade In Start", "Fade In Start");
		METASOUND_PARAM(InFadeInEnd, "Fade In End", "Fade In End");
		METASOUND_PARAM(InFadeOutStart, "Fade Out Start", "Fade Out Start");
		METASOUND_PARAM(InFadeOutEnd, "Fade Out End", "Fade Out End");
		METASOUND_PARAM(InAudioParam, "Audio In", "Input Audio Channel");
		METASOUND_PARAM(OutAudioParam, "Audio Out", "Audio Output");
	}

	FCBPOperator::FCBPOperator(const FOperatorSettings& InSettings,
		const FAudioBufferReadRef& InAudio,
		const FFloatReadRef& ValueIn,
		const FFloatReadRef& FadeInStartIn,
		const FFloatReadRef& FadeInEndIn,
		const FFloatReadRef& FadeOutStartIn,
		const FFloatReadRef& FadeOutEndIn)
		: AudioInput(InAudio),
		FloatIn(ValueIn),
		FadeInStart(FadeInStartIn),
		FadeInEnd(FadeInEndIn),
		FadeOutStart(FadeOutStartIn),
		FadeOutEnd(FadeOutEndIn),
		AudioOutput(FAudioBufferWriteRef::CreateNew(InSettings)),
		NumFramesPerBlock(InSettings.GetNumFramesPerBlock())
	{

	};

	void FCBPOperator::Execute()
	{
		FMemory::Memcpy(AudioOutput->GetData(), AudioInput->GetData(), sizeof(float) * AudioInput->Num());

		if (*FloatIn != FloatInPrev || bInit == false)
		{
			if (!bInit)
			{
				bInit = true;
			}

			float FadeInValue = FMath::GetMappedRangeValueClamped(FVector2D(*FadeInStart, *FadeInEnd), FVector2D(0.f, 1.f), *FloatIn);
			float FadeOutValue = FMath::GetMappedRangeValueClamped(FVector2D(*FadeOutStart, *FadeOutEnd), FVector2D(1.f, 0.f), *FloatIn);

			Amplitude = FadeInValue * FadeOutValue;

			Audio::ArrayFade(*AudioOutput, AmplitudePrev, Amplitude);

			FloatInPrev = *FloatIn;
			AmplitudePrev = Amplitude;
		}
		else
		{
			Audio::ArrayFade(*AudioOutput, AmplitudePrev, Amplitude);
		}
	}

	const FVertexInterface& FCBPOperator::DeclareVertexInterface()
	{
		using namespace ECBPNodeNames;

		static const FVertexInterface Interface(
			FInputVertexInterface(
				TInputDataVertexModel<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InFloatValue)),
				TInputDataVertexModel<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InFadeInStart)),
				TInputDataVertexModel<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InFadeInEnd)),
				TInputDataVertexModel<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InFadeOutStart)),
				TInputDataVertexModel<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InFadeOutEnd)),
				TInputDataVertexModel<FAudioBuffer>(METASOUND_GET_PARAM_NAME_AND_METADATA(InAudioParam))
			),
			FOutputVertexInterface(
				TOutputDataVertexModel<FAudioBuffer>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutAudioParam))
			)
		);

		return Interface;
	};

	const FNodeClassMetadata& FCBPOperator::GetNodeInfo()
	{
		auto CreateNodeClassMetadata = []() -> FNodeClassMetadata
			{
				FVertexInterface NodeInterface = DeclareVertexInterface();

				FNodeClassMetadata Metadata
				{
						{ TEXT("UE"), TEXT("CrossfadeByParam"), TEXT("Audio") },
						1, // Major Version
						0, // Minor Version
						METASOUND_LOCTEXT("CBPDisplayName", "Crossfade By Param (Mono)"),
						METASOUND_LOCTEXT("CPTestNodeDesc", "Fades in and out a single audio channel by a mapped range"),
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

	void FCBPOperator::BindInputs(FInputVertexInterfaceData& InOutVertexData)
	{
		using namespace ECBPNodeNames;
		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InFloatValue), FloatIn);
		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InFadeInStart), FadeInStart);
		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InFadeInEnd), FadeInEnd);
		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InFadeOutStart), FadeOutStart);
		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InFadeOutEnd), FadeOutEnd);
		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InAudioParam), AudioInput);
	}

	void FCBPOperator::BindOutputs(FOutputVertexInterfaceData& InOutVertexData)
	{
		using namespace ECBPNodeNames;
		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(OutAudioParam), AudioOutput);
	}

	TUniquePtr<IOperator> FCBPOperator::CreateOperator(const FCreateOperatorParams& InParams, FBuildErrorArray& OutErrors)
	{
		using namespace ECBPNodeNames;

		const Metasound::FDataReferenceCollection& InputCollection = InParams.InputDataReferences;
		const Metasound::FInputVertexInterface& InputInterface = DeclareVertexInterface().GetInputInterface();

		TDataReadReference<float> FloatInputA = InputCollection.GetDataReadReferenceOrConstructWithVertexDefault<float>(InputInterface, METASOUND_GET_PARAM_NAME(InFloatValue), InParams.OperatorSettings);
		TDataReadReference<float> FadeInStartFloat = InputCollection.GetDataReadReferenceOrConstructWithVertexDefault<float>(InputInterface, METASOUND_GET_PARAM_NAME(InFadeInStart), InParams.OperatorSettings);
		TDataReadReference<float> FadeInEndFloat = InputCollection.GetDataReadReferenceOrConstructWithVertexDefault<float>(InputInterface, METASOUND_GET_PARAM_NAME(InFadeInEnd), InParams.OperatorSettings);
		TDataReadReference<float> FadeOutStartFloat = InputCollection.GetDataReadReferenceOrConstructWithVertexDefault<float>(InputInterface, METASOUND_GET_PARAM_NAME(InFadeOutStart), InParams.OperatorSettings);
		TDataReadReference<float> FadeOutEndFloat = InputCollection.GetDataReadReferenceOrConstructWithVertexDefault<float>(InputInterface, METASOUND_GET_PARAM_NAME(InFadeOutEnd), InParams.OperatorSettings);

		FAudioBufferReadRef AudioIn1 = InputCollection.GetDataReadReferenceOrConstructWithVertexDefault<FAudioBuffer>(InputInterface, METASOUND_GET_PARAM_NAME(InAudioParam), InParams.OperatorSettings);

		//this class is FCBPOperator, which inherits from TExecutableOperator, which inherits from IOperator. IOperator type is returned
		return MakeUnique<FCBPOperator>(InParams.OperatorSettings, AudioIn1, FloatInputA, FadeInStartFloat, FadeInEndFloat, FadeOutStartFloat, FadeOutEndFloat);
	}

	// Register node
	METASOUND_REGISTER_NODE(FCBPNode);
}

#undef LOCTEXT_NAMESPACE
