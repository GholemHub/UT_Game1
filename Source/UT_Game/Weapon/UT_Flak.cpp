// Fill out your copyright notice in the Description page of Project Settings.


#include "../Weapon/UT_Flak.h"
#include "../Weapon/UT_Flak_Projectile.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Actor.h"
#include "../UT_GameCharacter.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Net/UnrealNetwork.h"

#include "Engine/Engine.h"


// Sets default values
AUT_Flak::AUT_Flak()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	//RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	//SkeletMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMeshComponent"));
	RootComponent = MeshComponent;
	//MeshComponent->SetupAttachment(RootComponent);

	FirePoint = CreateDefaultSubobject<USceneComponent>(TEXT("FirePoint"));
	FirePoint->SetupAttachment(MeshComponent);

	bReplicates = true;
	SetReplicateMovement(true);
}

// Called when the game starts or when spawned
void AUT_Flak::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void AUT_Flak::Tick(float DeltaTime)
{
	
	
}

void AUT_Flak::Fire()
{
	if (!bCanFire)
	{
		UE_LOG(LogTemp, Warning, TEXT("Cannot fire yet!"));
		return;
	}

	//UE_LOG(LogTemp, Warning, TEXT("Fire() called on %s, HasAuthority: %s"), *GetName(), HasAuthority() ? TEXT("true") : TEXT("false"));

	APlayerController* PC = GetWorld()->GetFirstPlayerController();

	if (!PC)
	{
		UE_LOG(LogTemp, Warning, TEXT("No Player Controller!"));
		return;
	}

	

	int32 ScreenWidth, ScreenHeight;
	PC->GetViewportSize(ScreenWidth, ScreenHeight);

	FVector2D ScreenCenter(ScreenWidth * 0.5f, ScreenHeight * 0.5f);
	FVector WorldLocation, WorldDirection;
	PC->DeprojectScreenPositionToWorld(ScreenCenter.X, ScreenCenter.Y, WorldLocation, WorldDirection);

	//FVector FirePointLocation = FirePoint->GetComponentLocation();
	FVector FirePointLocation = FirePoint->GetComponentLocation();
	FVector EndTrace = FirePointLocation + (WorldDirection * 10000.0f);

	FHitResult HitResult;
	FCollisionQueryParams TraceParams;
	TraceParams.AddIgnoredActor(this);
	TraceParams.AddIgnoredActor(GetOwner());

	bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, FirePointLocation, EndTrace, ECC_Visibility, TraceParams);

	FVector ShootDirection = bHit ? (HitResult.ImpactPoint - FirePointLocation).GetSafeNormal() : WorldDirection;

	if (!HasAuthority())
	{
		ServerFire(ShootDirection);
		return;
	}
	else {
		//GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Black, FString::Printf(TEXT("3")));
		ActuallyFire(ShootDirection);
	}
}


void AUT_Flak::MakeWeaponShake()
{
	FVector ShakeOffset = FVector(
		FMath::RandRange(-3.0f, 1.0f),
		FMath::RandRange(-3.0f, 1.0f),
		FMath::RandRange(-1.0f, 1.0f)
	);
	//MeshComponent->AddLocalOffset(ShakeOffset);
	MeshComponent->SetWorldLocation(MeshComponent->GetComponentLocation() + ShakeOffset);

	bShouldRecover = 1;
}

void AUT_Flak::ResetFire()
{
	bCanFire = true;

	AUT_GameCharacter* Player = GetPlayer();
	if (!Player || !Player->WeaponComponent) return;

	if (Player->WeaponComponent->bIsShoot)
	{
		Fire(); // Loop the firing again if still shooting
	}
}

AUT_GameCharacter* AUT_Flak::GetPlayer()
{
	AActor* OwnerActor = GetOwner();
	if (OwnerActor)
	{
		APawn* OwnerPawn = Cast<APawn>(OwnerActor);
		if (OwnerPawn)
		{
			AController* OwnerController = OwnerPawn->GetController();
			if (OwnerController && OwnerController->IsPlayerController())
			{
				auto Player = Cast<AUT_GameCharacter>(OwnerPawn);
				if (!Player) return nullptr;
				return Player;
			}
		}
	}
	return nullptr;
}


void AUT_Flak::ServerFire_Implementation(FVector_NetQuantizeNormal ShootDirection)
{
	GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Black, FString::Printf(TEXT("1")));

	ActuallyFire(ShootDirection);
}

void AUT_Flak::ActuallyFire(FVector& ShootDirection)
{
	bCanFire = false;

	if (!Projectile) return;

	AActor* OwnerActor = GetOwner();
	if (OwnerActor)
	{
		APawn* OwnerPawn = Cast<APawn>(OwnerActor);
		if (OwnerPawn)
		{
			AController* OwnerController = OwnerPawn->GetController();
			if (OwnerController && OwnerController->IsPlayerController())
			{
				auto Player = Cast<AUT_GameCharacter>(OwnerPawn);
				if (!Player) return;
				if (!GetWorld()) return;
				auto WEPON = Player->WeaponComponent->Weapon;
				if (!WEPON) return;
				FVector POINT = FVector(0,0,0);
				
				if (Player->IsLocallyControlled() && Player->WeaponMeshComponent->DoesSocketExist("Hole"))
				{
					POINT = Player->WeaponMeshComponent->GetSocketLocation("Hole");
				}
				else
				{
					POINT = MeshComponent->GetSocketLocation(FName("Hole"));
				}
				//POINT = MeshComponent->GetSocketLocation(FName("Hole"));
				FTransform SpawnTransform(FRotator::ZeroRotator, POINT);

				FActorSpawnParameters Params;
				Params.Owner = OwnerController;

				auto Pr = GetWorld()->SpawnActor<AUT_Flak_Projectile>(Projectile, SpawnTransform, Params);
				//GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green, FString::Printf(TEXT("SpawnProjectile %s"), *ROTATION.ToString()));
				
				DrawDebugSphere(
					GetWorld(),                   // World context
					POINT,                     // Center of the sphere (FVector)
					30.0f,                        // Radius
					12,                          // Segments
					FColor::Blue,                 // Color
					false,                       // Persistent lines (true = stays forever)
					10.0f                          // Life time in seconds
				);
				if (Pr)
				{
					//Pr->FinishSpawning(SpawnTransform);
					//Pr->MakeShot(SpawnTransform.GetLocation());
				}
			}
		}
	}

	GetWorldTimerManager().SetTimer(FireRateHandle, this, &AUT_Flak::ResetFire, 0.3f, false);
}

void AUT_Flak::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	//DOREPLIFETIME(AUT_Flak, FireRateHandle);
	//DOREPLIFETIME(AUT_Flak, bCanFire);
}

void AUT_Flak::On_bCanFire()
{
}

void AUT_Flak::On_FireRateHandle()
{
}
