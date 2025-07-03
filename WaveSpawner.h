
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WaveSettings.h"
#include "WaveSpawner.generated.h"

UCLASS()
class PROJECTSWAGGER_API AWaveSpawner : public AActor
{
	GENERATED_BODY()
	
public:
	AWaveSpawner();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	void SpawnWave(int32 WaveCount, const FWaveSettings& ManagerSettings);

private:
	void SpawnEnemy();

	UPROPERTY(EditAnywhere, Category = "Spawner")
	FWaveSettings SpawnerOverrideSettings;

	int32 EnemiesToSpawn = 0;
	int32 EnemiesSpawned = 0;

	FTimerHandle SpawnTimerHandle;
	FWaveSettings EffectiveSettings;

public:

	
	UPROPERTY(EditAnywhere, Category = "Spawner")
	bool bUseSpawnerOverride = false;
	UPROPERTY(EditAnywhere, Category = "Spawner")
	int SpawnerNumber;

	bool bRegistered = false;


};