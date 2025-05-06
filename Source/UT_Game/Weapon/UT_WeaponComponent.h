// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
//#include "../UT_GameCharacter.h"

#include "UT_WeaponComponent.generated.h"

class AUT_Flak;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UT_GAME_API UUT_WeaponComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UUT_WeaponComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	
public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	//void EquipWeapon(ACharacter*);
	void EquipWeapon(ACharacter*);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	TArray<TSubclassOf<AUT_Flak>> WeaponClasses;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapons")
	TArray<UStaticMesh*> WeaponMeshes;

	UPROPERTY(Replicated)
	AUT_Flak* Weapon;
	

	UPROPERTY(ReplicatedUsing = OnRep_CurrentWeaponIndex)
	int CurrentWeaponIndex = 0;

	UFUNCTION()
	void OnRep_CurrentWeaponIndex();



	UFUNCTION(BlueprintCallable)
	void SwitchWeapon();

	UFUNCTION(Server, Reliable)
	void Server_SwitchWeapon();
	
	virtual void FireStart();

	//void SwitchWeapon();
	UPROPERTY(Replicated)
	bool bIsShoot = false;


private:
	UFUNCTION()
	void HandleFirePressed();

	UFUNCTION()
	void HandleFireReleased();

	UFUNCTION(Server, Reliable)
	void Server_SetShooting(bool bShooting);

	void AttachWeaponToSocket(ACharacter* Owner);
	void SetMeshVisability(ACharacter* Owner);
};
