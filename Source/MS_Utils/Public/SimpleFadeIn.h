
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
#include "MetasoundAudioBuffer.h"


//------------------------------------------------------------------------------------
// FSimpleFIOperator
//------------------------------------------------------------------------------------

namespace Metasound
{
	class FSimpleFIOperator : public TExecutableOperator<FSimpleFIOperator>
	{
	public:
		FSimpleFIOperator(const FCreateOperatorParams& InSettings,
			const FTimeReadRef& FadeInTimeIn,
			const FTriggerReadRef& InTriggerEnter,
			const FTriggerReadRef& InTriggerReset);

		//static functions exist across the class and not instances. They cannot access member instance variables or non-static members
		//they can only access other static members (variables or methods) of the class.
		static const FVertexInterface& DeclareVertexInterface();

		static const FNodeClassMetadata& GetNodeInfo();

		virtual void BindInputs(FInputVertexInterfaceData& InOutVertexData) override;
		virtual void BindOutputs(FOutputVertexInterfaceData& InOutVertexData) override;

		// Used to instantiate a new runtime instance of your node
		static TUniquePtr<IOperator> CreateOperator(const FCreateOperatorParams& InParams, FBuildErrorArray& OutErrors);

		void Execute();

	private:

		FTriggerReadRef TriggerFadeIn;
		FTriggerReadRef ResetFadeIn;
		FTriggerWriteRef TriggerStartIn;
		FTriggerWriteRef TriggerFinished;
		FTimeReadRef FadeInTime;
		FFloatWriteRef OutEnvelope;
		//float SampleRate;
		float BlockRate;
		float FadeBlockCount = 0.f;
		float OutEnvelopeValue = 0.0f;
		float FadeCounter = 1.0f;
		bool ProcessFade = false;
		//int32 NumFramesPerBlock = 0;

	};

	//------------------------------------------------------------------------------------
	// FSimpleFINode
	//------------------------------------------------------------------------------------

	// Node Class - Inheriting from FNodeFacade is recommended for nodes that have a static FVertexInterface
	class FSimpleFINode : public FNodeFacade
	{
	public:
		//MetaSound frontend constructor
		FSimpleFINode(const FNodeInitData& InitData) : FNodeFacade(InitData.InstanceName, InitData.InstanceID,
			TFacadeOperatorClass<FSimpleFIOperator>())
		{
		}
	};

}


