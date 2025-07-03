// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemies/WaveSpawner.h"
#include "Enemies/EnemyBase.h"
#include "Enemies/EnemyManager.h"
#include "Enemies/WaveSpawnerManager.h"
#include "AkGameplayStatics.h"
#include "GameEvents.h"
#include "TimerManager.h"
#include "UI/ProjectSwaggerHUD.h"

// Sets default values
AWaveSpawner::AWaveSpawner()
{
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void AWaveSpawner::BeginPlay()
{
	Super::BeginPlay();
}

void AWaveSpawner::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	if (AProjectSwaggerHUD* GameHUD = Cast<AProjectSwaggerHUD>(GetWorld()->GetFirstPlayerController()->GetHUD()))
	{
		if (GameHUD->IsInUI())
		{
			// In UI, pause timer if isn't paused
			if (!GetWorldTimerManager().IsTimerPaused(SpawnTimerHandle))
			{
				GetWorldTimerManager().PauseTimer(SpawnTimerHandle);	
			}
		}
		else
		{
			// Outisde UI, unpause timer if paused
			if (GetWorldTimerManager().IsTimerPaused(SpawnTimerHandle))
			{
				GetWorldTimerManager().UnPauseTimer(SpawnTimerHandle);
			}
		}
	}
}


void AWaveSpawner::SpawnWave(int32 WaveCount, const FWaveSettings& ManagerSettings)
{
	EffectiveSettings = bUseSpawnerOverride ? SpawnerOverrideSettings : ManagerSettings;

	EnemiesToSpawn = EffectiveSettings.BaseEnemyCount;
	if (WaveCount % EffectiveSettings.DifficultyIncreaseEveryXWaves == 0)
	{
		UGameEvents::OnDifficultyIncreasing.Broadcast();
		EnemiesToSpawn += EffectiveSettings.EnemiesToAddPerDifficultyStep;
	}

	UAkGameplayStatics::PostEvent(EffectiveSettings.WaveSpawnAudioEvent, this, false, FOnAkPostEventCallback(), false);

	EnemiesSpawned = 0;
	GetWorldTimerManager().SetTimer(SpawnTimerHandle, this, &AWaveSpawner::SpawnEnemy, EffectiveSettings.TimeBetweenEnemies, true);
}

void AWaveSpawner::SpawnEnemy()
{
	if (EnemiesSpawned >= EnemiesToSpawn)
	{
		GetWorldTimerManager().ClearTimer(SpawnTimerHandle);
		return;
	}

	FVector2D Random2D = FMath::RandPointInCircle(EffectiveSettings.SpawnRadius);
	FVector SpawnLocation = FVector(GetActorLocation().X + Random2D.X, GetActorLocation().Y + Random2D.Y, GetActorLocation().Z);

	FActorSpawnParameters SpawnParams;
	

	if (AEnemyBase* SpawnedEnemy = GetWorld()->SpawnActor<AEnemyBase>(EffectiveSettings.EnemyClass, SpawnLocation, FRotator::ZeroRotator, SpawnParams))
	{
		if (AEnemyManager* EnemyMgr = AEnemyManager::Get(GetWorld()))
			AEnemyManager::RegisterEnemy(SpawnedEnemy);
		SpawnedEnemy->SetParentSpawner(this);
	}

	EnemiesSpawned++;
}