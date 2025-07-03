#pragma once

#include "CoreMinimal.h"
#include "AkGameplayTypes.h"
#include "WaveSettings.generated.h"

USTRUCT(BlueprintType)
struct FWaveSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave", meta=(ToolTip="Type of enemy that will spawn from this spawner."))
	TSubclassOf<class AEnemyBase> EnemyClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave", meta=(ToolTip="Starting number of enemies in each wave."))
	int32 BaseEnemyCount = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave", meta=(ToolTip="Range of distance from the spawner enemy will randomly spawn."))
	float SpawnRadius = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave", meta=(ToolTip="Time before the spawn to show a warning for the next spawn."))
	float SpawnShowWarningTime = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave", meta=(ToolTip="Time between wave spawns."))
	float SpawnDelay = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave", meta=(ToolTip="Time between enemy spawns."))
	float TimeBetweenEnemies = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave", meta=(ToolTip="How many waves occur before adding more enemies to each spawn."))
	int32 DifficultyIncreaseEveryXWaves = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave", meta=(ToolTip="How many enemies to add each time the difficulty increases."))
	int32 EnemiesToAddPerDifficultyStep = 2;

	//Audio
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio", meta=(ToolTip="Sound that plays when a wave spawns."))
	UAkAudioEvent* WaveSpawnAudioEvent = nullptr;
};