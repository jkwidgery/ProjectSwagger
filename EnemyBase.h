// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Environment/BorderWall.h"
#include "WaveSpawner.h"
#include "Components/HealthComponent.h"
#include "EnemyBase.generated.h"

constexpr ECollisionChannel ECC_Enemy = ECollisionChannel::ECC_GameTraceChannel1;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEnemyAttack);

UCLASS(BlueprintType, Blueprintable)
class PROJECTSWAGGER_API AEnemyBase : public AActor
{
	GENERATED_BODY()
	
public:	
	AEnemyBase();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skeletal Mesh")
	TObjectPtr<USkeletalMeshComponent> VisualMesh;
	
	// boolean for playing attack animation in blueprint
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Attack", meta=(DisplayName = "Is Attacking"))
	bool bIsAttacking = false;
	
protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	UAnimMontage* HitReactMontage;

public:	
	virtual void Tick(float DeltaTime) override;
	
	//Health Component
	UFUNCTION()
	void OnDeath();
	
	UFUNCTION(BlueprintCallable)
	void ApplySimpleDamage(float DamageAmount, AActor* DamageCauser);
	
	float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
	AController* EventInstigator, AActor* DamageCauser) override;
	
	UFUNCTION()
	void OnHealthChanged(float CurrentHealth, float CurrentMaxHealth);

	void DestroyedWall();

	void SetParentSpawner(AWaveSpawner* Spawner) { ParentSpawner = Spawner; }

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic")
	float MoveSpeed = 200.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float AttackRange = 60.0f;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	TObjectPtr<ABorderWall> TargetWall = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float DamageInterval = 3.0f;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Stats")
	UHealthComponent* HealthComponent;
	
	void AdjustMaxHealth(float Value, bool IsAdding);
	void TempAdjustMaxHealth(float Value, bool IsAdding);
	void ResetTempMaxHealth();


	UFUNCTION(BlueprintImplementableEvent, Category = "VFX")
	void PlayAttackVFX() const;

	
private:
	FTimerHandle DamageTimerHandle;

	void FindAndSetClosestWall();

	FVector TargetPoint;

	UFUNCTION(BlueprintCallable)
	void Attack() const;
	
	UPROPERTY()
	TObjectPtr<AWaveSpawner> ParentSpawner;

	void MoveTowardTarget(float DeltaTime);

};
