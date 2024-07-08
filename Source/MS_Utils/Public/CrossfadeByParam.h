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
// FCBPOperator
//------------------------------------------------------------------------------------

namespace Metasound
{
	class FCBPOperator : public TExecutableOperator<FCBPOperator>
	{
	public:
		FCBPOperator(const FOperatorSettings& InSettings, 
			const FAudioBufferReadRef& InAudio, 
			const FFloatReadRef& FadeInStartIn,
			const FFloatReadRef& FadeInEndIn,
			const FFloatReadRef& FadeOutStartIn,
			const FFloatReadRef& FadeOutEndIn,
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

	private:

		FFloatReadRef FloatIn;
		FFloatReadRef FadeInStart;
		FFloatReadRef FadeInEnd;
		FFloatReadRef FadeOutStart;
		FFloatReadRef FadeOutEnd;
		FAudioBufferReadRef AudioInput;
		FAudioBufferWriteRef AudioOutput;
		int32 NumFramesPerBlock = 0;
		float SignalOnePreviousGain = 0.f;
		float SignalTwoPreviousGain = 0.f;
		float FloatInPrev = 0.0f;
		float Amplitude = 0.0f;
		float AmplitudePrev = 0.0f;
		bool bInit = false;
	};

	//------------------------------------------------------------------------------------
	// FCBPNode
	//------------------------------------------------------------------------------------

	// Node Class - Inheriting from FNodeFacade is recommended for nodes that have a static FVertexInterface
	class FCBPNode : public FNodeFacade
	{
	public:
		//MetaSound frontend constructor
		FCBPNode(const FNodeInitData& InitData) : FNodeFacade(InitData.InstanceName, InitData.InstanceID,
			TFacadeOperatorClass<FCBPOperator>())
		{
		}
	};

}


