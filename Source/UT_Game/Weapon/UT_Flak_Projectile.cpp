#include "Components/StaticMeshComponent.h"
#include "../Weapon/UT_Flak_Projectile.h"
#include "Engine/Engine.h"


// Sets default values
AUT_Flak_Projectile::AUT_Flak_Projectile()
{
    bReplicates = true;
    SetReplicateMovement(true);

    PrimaryActorTick.bCanEverTick = true;

    MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
    RootComponent = MeshComponent;

    MeshComponent->SetSimulatePhysics(true); // So it reacts physically
    MeshComponent->SetNotifyRigidBodyCollision(true); // So we get hit 

    MeshComponent->OnComponentHit.AddDynamic(this, &AUT_Flak_Projectile::OnHit);

   

}

// Called when the game starts or when spawned
void AUT_Flak_Projectile::BeginPlay()
{
	Super::BeginPlay();
	
}

void AUT_Flak_Projectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	//GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, FString::Printf(TEXT("DESRTOY")));
    //Destroy();
    
}

// Called every frame
void AUT_Flak_Projectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
   
}

void AUT_Flak_Projectile::MakeShot(FVector ShootDirection)
{
    if (MeshComponent) // Ensure MeshComponent exists
    {
        if (ShootDirection.IsNearlyZero())
        {
            ShootDirection = GetActorForwardVector();
        }
        FVector Impulse = ShootDirection * 1000.0f; // Adjust impulse strength

        MeshComponent->AddImpulse(Impulse, NAME_None, true); // Apply impulse to the mesh
    }
}