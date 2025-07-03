#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WaveSettings.h"
#include "WaveSpawnerManager.generated.h"

class AWaveSpawner;

UCLASS()
class PROJECTSWAGGER_API AWaveSpawnerManager : public AActor
{
	GENERATED_BODY()
	
public:
	AWaveSpawnerManager();

	static AWaveSpawnerManager* Get(UWorld* World);

	UFUNCTION(BlueprintCallable)
	void RegisterSpawner(AWaveSpawner* Spawner);

	UFUNCTION(BlueprintCallable)
	void StartNextWave();

	UFUNCTION(BlueprintCallable)
	void ShowWarning();

	UFUNCTION(BlueprintCallable)
	int32 GetWaveCount() const { return CurrentWaveCount; }
	
	
protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	void SetWaveTimer();
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Waves")
	FWaveSettings DefaultWaveSettings;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Waves")
	TArray<int> SpawnersActivePerWave;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Waves")
	float LockRadius = 100.0f;
	
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard System", meta=(ToolTip="Starting chance that a hazard will occur when an enemy attacks."))
	float HazardTriggerChance = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard System", meta=(ToolTip="How many hazards may occur at once, up to how many active nodes there are."))
	int MaxHazardsPerAttack = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard System", meta=(ToolTip="How many more hazards may spawn when difficulty increases."))
	int HazardCountIncreaseStep = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard System", meta=(ToolTip="How much more likely hazards are to occur when difficulty increases."))
	float HazardChanceIncreaseStep = 5.0f;

	UFUNCTION()
	void OnEnemyAttackReceived();

	UFUNCTION()
	void OnDifficultyIncreasing();
	
private:
	UPROPERTY()
	TArray<TWeakObjectPtr<AWaveSpawner>> Spawners;

	int NumSpawnersInScene = 0;
	int NumSpawnersIncreasedDifficulty = 0;

	UPROPERTY()
	int32 CurrentWaveCount = 0;
	
	FTimerHandle WaveTimerHandle;
	FTimerHandle WaveWarningTimerHandle;

	int GetPlayersCurrentArea();
	
	static AWaveSpawnerManager* Instance;

};
