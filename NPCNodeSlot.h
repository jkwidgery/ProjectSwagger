// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NPCCharacter.h"
#include "GameplayTagContainer.h"
#include "BehaviorTree/BehaviorTree.h"
#include "Interactables/Resources/ResourceBase.h"
#include "Interfaces/InteractionInterface.h"
#include "NPCNodeSlot.generated.h"


class UHealthComponent;
class UWidgetComponent;


//Types of nodes
/*
UENUM(BlueprintType)
enum class ENPCNodeType : uint8
{
	SHOOTING,
	MECHANIC,
	GARDENING,
	CARPENTER,
	GATHER_RESOURCES,
	SUPPLY_MANAGER,
	TECHNICIAN
};
*/

USTRUCT(BlueprintType)
struct FNPCHazard
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Need")
	int MinQuantity = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Need")
	int MaxQuantity = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard")
	float MinFrequency = 5;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard")
	float MaxFrequency = 15;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Need")
	FGameplayTag ResourceTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard")
	TObjectPtr<UBehaviorTree> HazardBehaviorTree = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recovery")
	FGameplayTag HealingResourceTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recovery")
	float HealthHealedPerResource = 50.0f;

	// Runtime data (not exposed)
	int32 CurrentQuantityNeeded = 0;

	FTimerHandle ResourceTimerHandle;

	// Randomized spawn interval
	float GetNextNeedDelay() const
	{
		return FMath::FRandRange(MinFrequency, MaxFrequency);
	}

	int32 GetRandomQuantity() const
	{
		return FMath::RandRange(MinQuantity, MaxQuantity);
	}
};

UCLASS()
class PROJECTSWAGGER_API ANPCNodeSlot : public AActor, public IInteractionInterface
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ANPCNodeSlot();

	//Interactable
	virtual void BeginFocus() override{};
	virtual void EndFocus() override {};
	virtual void BeginInteract() override {};
	virtual void EndInteract() override {};

	//Health Component
	UFUNCTION()
	void OnDeath();
	
	float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
	AController* EventInstigator, AActor* DamageCauser) override;

	UFUNCTION()
	void OnHealthChanged(float CurrentHealth, float CurrentMaxHealth);

	UFUNCTION()
	void StartHazardTimer();

	UFUNCTION(BlueprintCallable)
	void ApplySimpleDamage(float DamageAmount, AActor* DamageCauser);


	UFUNCTION(BlueprintNativeEvent, Category = "Interaction")
	void OnPlayerEnter(AActor* OtherActor);
	virtual void OnPlayerEnter_Implementation(AActor* OtherActor);

	UFUNCTION(BlueprintNativeEvent, Category = "Enemy Detection")
	void OnEnemyEnter(AActor* OtherActor);
	virtual void OnEnemyEnter_Implementation(AActor* OtherActor);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void Interact(AProjectSwaggerCharacter* Player) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Properties")
	UStaticMeshComponent* NodeMeshComponent = nullptr;

	UPROPERTY()
	TObjectPtr<UBehaviorTree> NPCOldBehaviorTree { nullptr };
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC")
	TObjectPtr<UBehaviorTree> BehaviorTreeAsset = nullptr;
	
	UPROPERTY(EditAnywhere, Category = "NPC")
	FNPCHazard Hazard;

	// NPC currently manning this station
	UPROPERTY(BlueprintReadOnly, Category = "NPC")
	ANPCCharacter* OccupantNPC = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Properties")
	UHealthComponent* HealthComponent = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (BindWidget), Category = "Properties")
	UWidgetComponent* HealthBarWidget;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Properties")
	UInventoryComponent* InventoryComponent = nullptr;

	//Hazards and resource needs
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<class USphereComponent> InteractionSphere;//so overlapping auto-delivers resources, if in inventory

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<class USphereComponent> DetectionSphere;

	UFUNCTION()
	void TriggerHazard();

	UFUNCTION()
	void OnPlayerOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
								   UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
								   bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnEnemyOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
								   UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
								   bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnResourceDelivered(FGameplayTag& ResourceType, int32 Quantity);

	UFUNCTION()
	bool HasRequiredResources(const AProjectSwaggerCharacter* Player, TArray<AResourceBase*>& ItemsToRemove) const;

	UFUNCTION()
	bool TakeHealingResources(AProjectSwaggerCharacter* Player);


	UFUNCTION(BlueprintImplementableEvent, Category = "State")
	void OnDisabled();

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	UPROPERTY(BlueprintReadWrite, Category = "Properties")
	bool bIsOccupied = false;

	UPROPERTY(BlueprintReadWrite, Category = "Properties")
	bool bIsHazardActive = false;

	UPROPERTY(BlueprintReadWrite, Category = "Properties")
	
	bool bHazardScheduled = false;

	UPROPERTY(BlueprintReadWrite, Category = "Properties")
	bool bIsDisabled = false;

	static unsigned int NumActiveNodes;
};
