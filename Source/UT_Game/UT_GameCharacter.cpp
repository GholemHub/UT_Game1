// Copyright Epic Games, Inc. All Rights Reserved.

#include "UT_GameCharacter.h"
#include "UT_GameProjectile.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Weapon/UT_WeaponComponent.h"
#include "Engine/LocalPlayer.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

//////////////////////////////////////////////////////////////////////////
// AUT_GameCharacter

AUT_GameCharacter::AUT_GameCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);
	WeaponComponent = CreateDefaultSubobject<UUT_WeaponComponent>(TEXT("UTWeaponComponent"));
	// Create a CameraComponent	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->SetRelativeLocation(FVector(-10.f, 0.f, 60.f)); // Position the camera
	FirstPersonCameraComponent->bUsePawnControlRotation = true;
	
	
	
	WeaponMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMeshComponent"));
	WeaponMeshComponent->SetupAttachment(FirstPersonCameraComponent);

	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh1P"));
	Mesh1P->SetOnlyOwnerSee(true);
	Mesh1P->SetupAttachment(FirstPersonCameraComponent);
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->CastShadow = false;
	Mesh1P->SetRelativeLocation(FVector(-30.f, 0.f, -150.f));

}

//////////////////////////////////////////////////////////////////////////// Input

void AUT_GameCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	// Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

void AUT_GameCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{	
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AUT_GameCharacter::Move);

		//Weapon
		EnhancedInputComponent->BindAction(SwitchWeaponAction, ETriggerEvent::Triggered, this, &AUT_GameCharacter::SwitchWeapon);
		EnhancedInputComponent->BindAction(FireStartAction, ETriggerEvent::Started, this, &AUT_GameCharacter::Fire);
		EnhancedInputComponent->BindAction(FireStartAction, ETriggerEvent::Started, this, &AUT_GameCharacter::HandleFirePressed);
		EnhancedInputComponent->BindAction(FireStartAction, ETriggerEvent::Completed, this, &AUT_GameCharacter::HandleFireReleased);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AUT_GameCharacter::Look);
		
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input Component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}


void AUT_GameCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add movement 
		AddMovementInput(GetActorForwardVector(), MovementVector.Y);
		AddMovementInput(GetActorRightVector(), MovementVector.X);
	}
}

void AUT_GameCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void AUT_GameCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (!WeaponComponent) return;
}

void AUT_GameCharacter::Fire(const FInputActionValue& Value)
{
	GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Red, TEXT("Character Fire"));
    auto Weapon = Cast<UUT_WeaponComponent>(WeaponComponent);
	if (!Weapon) return;
	//Weapon->FireStart();
}

void AUT_GameCharacter::HandleFirePressed()
{
	// Broadcast to listeners
	OnFirePressed.Broadcast();

	// Optional: Trigger immediate weapon fire if needed
}

void AUT_GameCharacter::HandleFireReleased()
{
	// Broadcast to listeners
	OnFireReleased.Broadcast();

	// Optional: Stop firing, stop charging, etc.
}


void AUT_GameCharacter::SwitchWeapon()
{
	if (HasAuthority())
	{
		// We are already the server
		Server_SwitchWeapon();
	}
	else
	{
		// Tell server we want to switch
		Server_SwitchWeapon();
	}
}

void AUT_GameCharacter::Server_SwitchWeapon_Implementation()
{
	if (WeaponComponent)
	{
		WeaponComponent->SwitchWeapon();
	}
}


FVector AUT_GameCharacter::GetMuzzleLocation() const
{
	if (IsLocallyControlled() && WeaponMeshComponent && WeaponMeshComponent->DoesSocketExist("Hole"))
	{
		return WeaponMeshComponent->GetSocketLocation("Hole");
	}
	if (WeaponMeshComponent && WeaponMeshComponent->DoesSocketExist("Hole"))
	{
		return WeaponMeshComponent->GetSocketLocation("Hole");
	}
	return GetActorLocation(); // Fallback
}

