// Copyright Dale Grinsell 2024. All Rights Reserved. 

#pragma once

#include "CoreMinimal.h"

#include "MetasoundExecutableOperator.h"
#include "Internationalization/Text.h"
#include "MetasoundPrimitives.h"
#include "MetasoundTime.h"
#include "MetasoundNodeRegistrationMacro.h"
#include "MetasoundStandardNodesNames.h" 
#include "MetasoundFacade.h"
#include "MetasoundParamHelper.h" 


//------------------------------------------------------------------------------------
// FEPXFOperator
//------------------------------------------------------------------------------------

namespace Metasound
{
	class FEPXFOperator : public TExecutableOperator<FEPXFOperator>
	{
	public:
		FEPXFOperator(const FOperatorSettings& InSettings, 
			const FAudioBufferReadRef& InAudio, 
			const FAudioBufferReadRef& InAudio2, 
			const FFloatReadRef& ValueIn);

		//UFUNCTION()
		//static functions exist across the class and not instances. They cannot access member instance variables or non-static members
		//they can only access other static members (variables or methods) of the class.
		static const FVertexInterface& DeclareVertexInterface();

		//UFUNCTION()
		static const FNodeClassMetadata& GetNodeInfo();

		virtual void BindInputs(FInputVertexInterfaceData& InOutVertexData) override;
		virtual void BindOutputs(FOutputVertexInterfaceData& InOutVertexData) override;

		//UFUNCTION
		// Used to instantiate a new runtime instance of your node
		static TUniquePtr<IOperator> CreateOperator(const FCreateOperatorParams& InParams, FBuildErrorArray& OutErrors);

		//UFUNCTION()
		void Execute();

		//UFUNCTION()
		void MixInInput(FAudioBufferReadRef& InBuffer, TArrayView<float>& OutBufferView, float PrevGain, float NewGain);

	private:

		FFloatReadRef FloatIn;
		FAudioBufferReadRef AudioInput;
		FAudioBufferReadRef AudioInput2;
		FAudioBufferWriteRef AudioOutput;
		int32 NumFramesPerBlock = 0;
		float SignalOnePreviousGain = 0.f;
		float SignalTwoPreviousGain = 0.f;
		float FloatInPrev = 1.1f;
		float SignalOneFloat;
		float SignalTwoFloat;
	};

	//------------------------------------------------------------------------------------
	// FEPXFNode
	//------------------------------------------------------------------------------------

	// Node Class - Inheriting from FNodeFacade is recommended for nodes that have a static FVertexInterface
	class FEPXFNode : public FNodeFacade
	{
	public:
		//MetaSound frontend constructor
		FEPXFNode(const FNodeInitData& InitData) : FNodeFacade(InitData.InstanceName, InitData.InstanceID,
			TFacadeOperatorClass<FEPXFOperator>())
		{
		}
	};

}


