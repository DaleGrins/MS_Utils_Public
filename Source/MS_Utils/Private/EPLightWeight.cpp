// Copyright Dale Grinsell 2024. All Rights Reserved. 

#include "EPLightWeight.h"

#include "DSP/FloatArrayMath.h"
#include "MetasoundStandardNodesCategories.h"

#define LOCTEXT_NAMESPACE "MetasoundStandardNodes_EPCrossfade_Lightweight"

namespace Metasound
{
	//the below stores name and tooltip information for each input/output pin - Name and then description.

	namespace EPXFNodeNames
	{
		METASOUND_PARAM(InFloatValue, "Crossfade Value", "Crossfade Value");
		METASOUND_PARAM(InAudioParam, "Audio In 1", "Input Audio Channel 1");
		METASOUND_PARAM(InAudioParam2, "Audio In 2", "Input Audio Channel 2");
		METASOUND_PARAM(OutAudioParam, "Audio Out", "Audio Output");
	}

	FEPXFOperator::FEPXFOperator(const FOperatorSettings& InSettings,
		const FAudioBufferReadRef& InAudio,
		const FAudioBufferReadRef& InAudio2,
		const FFloatReadRef& ValueIn)
		: AudioInput(InAudio),
		AudioInput2(InAudio2),
		FloatIn(ValueIn),
		AudioOutput(FAudioBufferWriteRef::CreateNew(InSettings)),
		NumFramesPerBlock(InSettings.GetNumFramesPerBlock())
	{

	};

	void FEPXFOperator::Execute()
	{
		if (*FloatIn != FloatInPrev)
		{
			SignalOneFloat = FMath::Clamp(FMath::Cos(*FloatIn * HALF_PI), 0.f, 1.f);
			SignalTwoFloat = FMath::Clamp(FMath::Cos((1 - *FloatIn) * (HALF_PI)), 0.f, 1.f);
		}
	
			FAudioBuffer& OutputBuffer = *AudioOutput;
			OutputBuffer.Zero();
			TArrayView<float> OutAudioBufferView(OutputBuffer.GetData(), OutputBuffer.Num());

			MixInInput(AudioInput, OutAudioBufferView, SignalOnePreviousGain, SignalOneFloat);
			MixInInput(AudioInput2, OutAudioBufferView, SignalTwoPreviousGain, SignalTwoFloat);
		
		if (*FloatIn != FloatInPrev)
		{
			FloatInPrev = *FloatIn;
			SignalOnePreviousGain = SignalOneFloat;
			SignalTwoPreviousGain = SignalTwoFloat;
		}
	}

	void FEPXFOperator::MixInInput(FAudioBufferReadRef& InBuffer, TArrayView<float>& OutBufferView, float PrevGain, float NewGain)
	{
		TArrayView<const float> BufferView((*InBuffer).GetData(), NumFramesPerBlock);
		Audio::ArrayMixIn(BufferView, OutBufferView, PrevGain, NewGain);
	}

	const FVertexInterface& FEPXFOperator::DeclareVertexInterface()
	{
		using namespace EPXFNodeNames;

		static const FVertexInterface Interface(
			FInputVertexInterface(
				TInputDataVertexModel<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InFloatValue)),
				TInputDataVertexModel<FAudioBuffer>(METASOUND_GET_PARAM_NAME_AND_METADATA(InAudioParam)),
				TInputDataVertexModel<FAudioBuffer>(METASOUND_GET_PARAM_NAME_AND_METADATA(InAudioParam2))
			),
			FOutputVertexInterface(
				TOutputDataVertexModel<FAudioBuffer>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutAudioParam))
			)
		);

		return Interface;
	};

	const FNodeClassMetadata& FEPXFOperator::GetNodeInfo()
	{
		auto CreateNodeClassMetadata = []() -> FNodeClassMetadata
			{
				FVertexInterface NodeInterface = DeclareVertexInterface();

				FNodeClassMetadata Metadata
				{
						{ TEXT("UE"), TEXT("EPLight"), TEXT("Audio") },
						1, // Major Version
						0, // Minor Version
						METASOUND_LOCTEXT("EPTestDisplayName", "EP Crossfade Lightweight"),
						METASOUND_LOCTEXT("EPTestNodeDesc", "Crossfades between two audio channels by the cos equal power function"),
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

	void FEPXFOperator::BindInputs(FInputVertexInterfaceData& InOutVertexData)
	{
		using namespace EPXFNodeNames;
		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InFloatValue), FloatIn);
		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InAudioParam), AudioInput);
		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(InAudioParam2), AudioInput2);
	}

	void FEPXFOperator::BindOutputs(FOutputVertexInterfaceData& InOutVertexData)
	{
		using namespace EPXFNodeNames;
		InOutVertexData.BindReadVertex(METASOUND_GET_PARAM_NAME(OutAudioParam), AudioOutput);
	}

	TUniquePtr<IOperator> FEPXFOperator::CreateOperator(const FCreateOperatorParams& InParams, FBuildErrorArray& OutErrors)
	{
		using namespace EPXFNodeNames;

		const Metasound::FDataReferenceCollection& InputCollection = InParams.InputDataReferences;
		const Metasound::FInputVertexInterface& InputInterface = DeclareVertexInterface().GetInputInterface();

		TDataReadReference<float> FloatInputA = InputCollection.GetDataReadReferenceOrConstructWithVertexDefault<float>(InputInterface, METASOUND_GET_PARAM_NAME(InFloatValue), InParams.OperatorSettings);
		FAudioBufferReadRef AudioIn1 = InputCollection.GetDataReadReferenceOrConstructWithVertexDefault<FAudioBuffer>(InputInterface, METASOUND_GET_PARAM_NAME(InAudioParam), InParams.OperatorSettings);
		FAudioBufferReadRef AudioIn2 = InputCollection.GetDataReadReferenceOrConstructWithVertexDefault<FAudioBuffer>(InputInterface, METASOUND_GET_PARAM_NAME(InAudioParam2), InParams.OperatorSettings);

		//this class is FEPXFOperator, which inherits from TExecutableOperator, which inherits from IOperator. IOperator type is returned
		return MakeUnique<FEPXFOperator>(InParams.OperatorSettings, AudioIn1, AudioIn2, FloatInputA);
	}

	// Register node
	METASOUND_REGISTER_NODE(FEPXFNode);
}

#undef LOCTEXT_NAMESPACE
