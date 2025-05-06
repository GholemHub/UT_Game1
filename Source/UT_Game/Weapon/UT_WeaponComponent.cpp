// Fill out your copyright notice in the Description page of Project Settings.


#include "../Weapon/UT_WeaponComponent.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "../Weapon/UT_Flak.h"
#include "../UT_GameCharacter.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

// Sets default values for this component's properties
UUT_WeaponComponent::UUT_WeaponComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true); // Enable replication
}


// Called when the game starts
void UUT_WeaponComponent::BeginPlay()
{
	Super::BeginPlay();

	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!OwnerCharacter) return;

    EquipWeapon(OwnerCharacter);

	AUT_GameCharacter* Player = Cast<AUT_GameCharacter>(GetOwner());
	if (Player)
	{
		Player->OnFirePressed.AddDynamic(this, &UUT_WeaponComponent::HandleFirePressed);
		Player->OnFireReleased.AddDynamic(this, &UUT_WeaponComponent::HandleFireReleased);
	}
}

void UUT_WeaponComponent::EquipWeapon(ACharacter* Owner)
{
    if (!WeaponClasses.IsValidIndex(CurrentWeaponIndex) || !Owner) return;

    if (Weapon && IsValid(Weapon))
    {
        Weapon->Destroy();
        Weapon = nullptr;
    }
    AUT_GameCharacter* Player = Cast<AUT_GameCharacter>(Owner);
    if (!Player) return;

    AttachWeaponToSocket(Owner);

    if (WeaponMeshes.IsValidIndex(CurrentWeaponIndex)) {
        Player->WeaponMeshComponent->SetStaticMesh(WeaponMeshes[CurrentWeaponIndex]);
    }
    SetMeshVisability(Owner);

}

//void UUT_WeaponComponent::OnRep_Weapon()
//{
//  
//}

void UUT_WeaponComponent::OnRep_CurrentWeaponIndex()
{
    if (Weapon && IsValid(Weapon))
    {
        Weapon->Destroy();
        Weapon = nullptr;
    }

    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    if (OwnerCharacter)
    {
        EquipWeapon(OwnerCharacter);
    }
}

void UUT_WeaponComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UUT_WeaponComponent, CurrentWeaponIndex);
	DOREPLIFETIME(UUT_WeaponComponent, Weapon);
    DOREPLIFETIME(UUT_WeaponComponent, bIsShoot);


}

void UUT_WeaponComponent::SwitchWeapon()
{
    if (GetOwnerRole() < ROLE_Authority) // only client
    {
        Server_SwitchWeapon(); // Ask server
    }
    else
    {
        // Directly switch on server
        CurrentWeaponIndex = (CurrentWeaponIndex + 1) % WeaponClasses.Num();
        EquipWeapon(Cast<ACharacter>(GetOwner()));
    }
}

void UUT_WeaponComponent::Server_SwitchWeapon_Implementation()
{
    CurrentWeaponIndex = (CurrentWeaponIndex + 1) % WeaponClasses.Num();
    EquipWeapon(Cast<ACharacter>(GetOwner()));
}

void UUT_WeaponComponent::HandleFirePressed()
{
    if (!GetOwner()->HasAuthority())
    {
        Server_SetShooting(true); // Let the server handle firing
    }
    else
    {
        bIsShoot = true;
        if (Weapon)
        {
            Weapon->Fire(); // Server fires if authority
        }
    }
}

void UUT_WeaponComponent::HandleFireReleased()
{
    GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Green, TEXT("DELEGATE 2Released"));
    if (!GetOwner()->HasAuthority())
    {
        Server_SetShooting(false);
    }
    bIsShoot = false;
}

void UUT_WeaponComponent::Server_SetShooting_Implementation(bool bShooting)
{
    bIsShoot = bShooting;
    if (bShooting && Weapon)
    {
        Weapon->Fire(); // Server executes fire
    }
}

void UUT_WeaponComponent::AttachWeaponToSocket(ACharacter* Owner)
{
    AUT_GameCharacter* Player = Cast<AUT_GameCharacter>(Owner);
    if (!Player) return;
    USkeletalMeshComponent* SkeletalMesh = Player->GetMesh();
    if (!SkeletalMesh) return;

    FName SocketName = TEXT("hand_rSocket");

    FVector SpawnLocation = SkeletalMesh->GetSocketLocation(SocketName);
    FRotator SpawnRotation = SkeletalMesh->GetSocketRotation(SocketName);

    TSubclassOf<AUT_Flak> WeaponClass = WeaponClasses[CurrentWeaponIndex];
    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = Player;
    SpawnParams.Instigator = Player->GetInstigator();
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    Weapon = GetWorld()->SpawnActor<AUT_Flak>(WeaponClass, SpawnLocation, SpawnRotation, SpawnParams);
    if (!Weapon) return;

    Weapon->SetReplicates(true);
    Weapon->SetReplicateMovement(true);
    Weapon->AttachToComponent(SkeletalMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, SocketName);
}
void UUT_WeaponComponent::SetMeshVisability(ACharacter* Owner)
{
    AUT_GameCharacter* Player = Cast<AUT_GameCharacter>(Owner);
    if (!Player) return;

    //if (!Player->HasAuthority()) {
        if (Player->IsLocallyControlled())
        {
            Weapon->MeshComponent->SetVisibility(false);
            Player->WeaponMeshComponent->SetVisibility(true);
        }
        else
        {

            Weapon->MeshComponent->SetVisibility(true);
            Player->WeaponMeshComponent->SetVisibility(false);
        }
    //}
    //else {
    //    if (Player->IsLocallyControlled())
    //    {
    //        Weapon->MeshComponent->SetVisibility(false);
    //        Player->WeaponMeshComponent->SetVisibility(true);
    //    }
    //    else
    //    {

    //        Weapon->MeshComponent->SetVisibility(true);
    //        //Player->WeaponMeshComponent->SetVisibility(false);
    //    }
    //}
    
    
}
// Called every frame
void UUT_WeaponComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
}
void UUT_WeaponComponent::FireStart()
{
    if (!Weapon) return;
    Weapon->Fire();   
    
}
