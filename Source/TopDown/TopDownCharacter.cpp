// Copyright Epic Games, Inc. All Rights Reserved.

#include "TopDownCharacter.h"
#include "JetPackComponent.h"
#include "TopDownPlayerHUD.h"
#include "MathUtil.h"
#include "Blueprint/UserWidget.h"
#include "Camera/CameraComponent.h"
#include "Components/DecalComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Materials/Material.h"
#include "UObject/ConstructorHelpers.h"
#include "TopDown/TopDownPlayerController.h"

ATopDownCharacter::ATopDownCharacter()
{
	// Set size for player capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	GetCharacterMovement()->RotationRate = FRotator(0.f, 640.f, 0.f);
	GetCharacterMovement()->bConstrainToPlane = true;
	GetCharacterMovement()->bSnapToPlaneAtStart = true;

	// Create a camera boom...
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->SetUsingAbsoluteRotation(true); // Don't want arm to rotate when character does
	CameraBoom->SetRelativeRotation(FRotator(-60.f, 0.f, 0.f));
	CameraBoom->bDoCollisionTest = false; // Don't want to pull camera in when it collides with level

	// Create a camera...
	TopDownCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
	TopDownCameraComponent->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	TopDownCameraComponent->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// HUD var intialization
	TopDownPlayerHUDClass = nullptr;
	TopDownPlayerHUD = nullptr;

	// Activate ticking in order to update the cursor every frame.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

void ATopDownCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Create HUD
	ATopDownPlayerController* TopDownPlayerController = GetController< ATopDownPlayerController >();
	TopDownPlayerHUD = CreateWidget< UTopDownPlayerHUD >(TopDownPlayerController, TopDownPlayerHUDClass);
	if (TopDownPlayerHUD != nullptr) {
		TopDownPlayerHUD->AddToPlayerScreen();
	}

	// Search for JetPackComponent in Attached Actors
	TArray< AActor* > AttachedActors;
	GetAttachedActors(AttachedActors);

	// Cached all the Niagara Components of attached actors
	for (AActor* Actor : AttachedActors)
	{
		for (UActorComponent* Component : Actor->GetComponents())
		{
			if (UJetPackComponent* JetPackComponent = Cast< UJetPackComponent >(Component))
			{
				AttachedJetPackComponent = JetPackComponent;
			}
		}
	}

}

void ATopDownCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	// Unattach HUD
	if (TopDownPlayerHUD)
	{
		TopDownPlayerHUD->RemoveFromParent();
		TopDownPlayerHUD = nullptr;
	}
}

void ATopDownCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UCharacterMovementComponent* CharacterMovementComponent = GetCharacterMovement();
	if (CharacterMovementComponent == nullptr)
	{
		return;
	}

	// Apply the Jetpacks requested impulse to the CharacterMovementComponent if active
	if (AttachedJetPackComponent != nullptr)
	{
		if (AttachedJetPackComponent->IsJetPackActive())
		{
			CharacterMovementComponent->SetMovementMode(EMovementMode::MOVE_Flying);
			CharacterMovementComponent->AddImpulse(AttachedJetPackComponent->GetDesiredImpulse(), true);
		}
		else if (CharacterMovementComponent->MovementMode == EMovementMode::MOVE_Flying)
		{
			CharacterMovementComponent->SetMovementMode(EMovementMode::MOVE_Falling);
		}

		TopDownPlayerHUD->SetFuel(AttachedJetPackComponent->GetFuel(), AttachedJetPackComponent->GetMaximumFuel());
	}
}
