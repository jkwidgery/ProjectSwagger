#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Environment/BorderWall.h"
#include "EnemyManager.generated.h"

class AEnemyBase;

UCLASS()
class PROJECTSWAGGER_API AEnemyManager : public AActor
{
	GENERATED_BODY()
	
public:	
	AEnemyManager();

protected:
	virtual void BeginPlay() override;

public:	
	UFUNCTION()
	static void RegisterEnemy(AEnemyBase* Enemy);

	UFUNCTION()
	static void UnregisterEnemy(AEnemyBase* Enemy);

	static AEnemyManager* Get(UWorld* World);

	UFUNCTION()
	static const TArray<AEnemyBase*>& GetAllEnemies();

	// Stats changes for every enemy
	UFUNCTION()
	static void TempAdjustMaxHealthForAll(float Value, bool IsAdding);
	
	UFUNCTION()
	static void AdjustMaxHealthForAll(float Value, bool IsAdding);
	
	UFUNCTION()
	static void ResetTempMaxHealthForAll();

private:

	static TArray<AEnemyBase*> ActiveEnemies;

	static AEnemyManager* Instance;
	
};
