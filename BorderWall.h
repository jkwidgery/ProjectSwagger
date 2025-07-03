// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/HealthComponent.h"
#include "BorderWall.generated.h"

UCLASS()
class PROJECTSWAGGER_API ABorderWall : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ABorderWall();

	UFUNCTION()
	void OnDeath();

	float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
		
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Components")
	TObjectPtr<UHealthComponent> HealthComponent;

	UFUNCTION(BlueprintImplementableEvent, Category = "State")
	void OnDisabled();

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UStaticMeshComponent* GetWallMesh() const { return WallMesh; }

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* WallMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall Properties")
	float Health = 100.0f;

};
