
#include "Enemies/WaveSpawnerManager.h"
#include "Enemies/WaveSpawner.h"
#include "GameEvents.h"
#include "Algo/RandomShuffle.h"
#include "Interactables/Base/BPI_GateControl.h"
#include "NPCs/NPCNodeSlot.h"
#include "ProjectSwagger/ProjectSwaggerCharacter.h"
#include "AkAudio/Classes/AkGameplayStatics.h"
#include "Environment/MiasmaManager.h"
#include "Kismet/GameplayStatics.h"
#include "UI/ProjectSwaggerHUD.h"

AWaveSpawnerManager* AWaveSpawnerManager::Instance = nullptr;


AWaveSpawnerManager* AWaveSpawnerManager::Get(UWorld* World)
{
	return Instance;
}

void AWaveSpawnerManager::SetWaveTimer()
{
	GetWorld()->GetTimerManager().SetTimer(
	WaveTimerHandle,
	this,
	&AWaveSpawnerManager::StartNextWave,
	DefaultWaveSettings.SpawnDelay,
	true
	);

	GetWorld()->GetTimerManager().SetTimer(
	WaveWarningTimerHandle,
	this,
	&AWaveSpawnerManager::ShowWarning,
	DefaultWaveSettings.SpawnDelay - DefaultWaveSettings.SpawnShowWarningTime,
	false
	);
}

AWaveSpawnerManager::AWaveSpawnerManager()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AWaveSpawnerManager::BeginPlay()
{
	Super::BeginPlay();
	Instance = this;

	Spawners.Empty();
	
	GetWorld()->GetTimerManager().SetTimerForNextTick(this, &AWaveSpawnerManager::SetWaveTimer);


	TArray<AActor*> FoundSpawners;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AWaveSpawner::StaticClass(), FoundSpawners);
	NumSpawnersInScene = FoundSpawners.Num();

	for (AActor* Actor : FoundSpawners)
	{
		if (AWaveSpawner* Spawner = Cast<AWaveSpawner>(Actor))
		{
			RegisterSpawner(Spawner);
		}
	}

	Spawners.Sort([](const TWeakObjectPtr<AWaveSpawner>& a, const TWeakObjectPtr<AWaveSpawner>& b) { return a.Get()->SpawnerNumber < b.Get()->SpawnerNumber; });

	UGameEvents::OnEnemyAttack.AddDynamic(this, &AWaveSpawnerManager::OnEnemyAttackReceived);
	UGameEvents::OnDifficultyIncreasing.AddDynamic(this, &AWaveSpawnerManager::OnDifficultyIncreasing);
}

void AWaveSpawnerManager::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	
	if (AProjectSwaggerHUD* GameHUD = Cast<AProjectSwaggerHUD>(GetWorld()->GetFirstPlayerController()->GetHUD()))
	{
		if (GameHUD->IsInUI())
		{
			// In UI, pause timer if isn't paused
			if (!GetWorldTimerManager().IsTimerPaused(WaveTimerHandle))
			{
				GetWorldTimerManager().PauseTimer(WaveTimerHandle);
			}
			
			if (!GetWorldTimerManager().IsTimerPaused(WaveWarningTimerHandle))
			{
				GetWorldTimerManager().PauseTimer(WaveWarningTimerHandle);	
			}
		}
		else
		{
			// Outisde UI, unpause timer if paused
			if (GetWorldTimerManager().IsTimerPaused(WaveTimerHandle))
			{
				GetWorldTimerManager().UnPauseTimer(WaveTimerHandle);
			}
			
			if (GetWorldTimerManager().IsTimerPaused(WaveWarningTimerHandle))
			{
				GetWorldTimerManager().UnPauseTimer(WaveWarningTimerHandle);
			}
		}
	}
}

void AWaveSpawnerManager::RegisterSpawner(AWaveSpawner* Spawner)
{
	Spawners.RemoveAll([](const TWeakObjectPtr<AWaveSpawner>& Ptr)
	{
		return !Ptr.IsValid();
	});
	
	if (IsValid(Spawner))
	{
		Spawners.AddUnique(Spawner);
	}
}

int AWaveSpawnerManager::GetPlayersCurrentArea()
{
	AActor* Player = UGameplayStatics::GetActorOfClass(GetWorld(), AProjectSwaggerCharacter::StaticClass());
	FVector2D PlayerPosition(Player->GetActorLocation().X, Player->GetActorLocation().Y);
	if (PlayerPosition.Length() < 1700)
	{
		return -1;
	}
	
	FVector2D Unit(-1,0);
	PlayerPosition.Normalize();
	float DotProduct = FVector2D::DotProduct(Unit, PlayerPosition);
	float Angle = FMath::RadiansToDegrees(FMath::Acos(DotProduct));

	int Area = static_cast<int>((Angle + 22.5) / 45) % 5;

	if (PlayerPosition.Y > 0 && Area != 0)
	{
		Area = 8 - Area;
	}
	return Area;
}

void AWaveSpawnerManager::StartNextWave()
{
	if (!GetWorldTimerManager().IsTimerActive(WaveTimerHandle))
		return;

	TArray<int> SpawnersToUse;
	
	int ActiveSpawners;
	if (SpawnersActivePerWave.Num() > 0)
	{
		if (CurrentWaveCount < SpawnersActivePerWave.Num())
		{
			ActiveSpawners = SpawnersActivePerWave[CurrentWaveCount];
		}
		else
		{
			ActiveSpawners = SpawnersActivePerWave[SpawnersActivePerWave.Num() - 1];
		}
	}
	else
	{
		ActiveSpawners = 1;
	}

	TArray<int> ValidAreas = {0,1,2,3,4,5,6,7};

	int InvalidArea = GetPlayersCurrentArea();
	ValidAreas.RemoveSwap(InvalidArea);
	
	while (ActiveSpawners > SpawnersToUse.Num() && ValidAreas.Num() > 0)
	{
		int Index = rand() % ValidAreas.Num();
		SpawnersToUse.Add(ValidAreas[Index]);
		ValidAreas.RemoveAtSwap(Index);
	}

	TArray<AActor*> GateActors;
	UGameplayStatics::GetAllActorsWithInterface(GetWorld(), UBPI_GateControl::StaticClass(), GateActors);
	
	for (int SpawnerNum : SpawnersToUse)
	{
		if (SpawnerNum <= Spawners.Num())
		{
			Spawners[SpawnerNum]->SpawnWave(CurrentWaveCount, DefaultWaveSettings);

			for (AActor* Gate : GateActors)
			{
				float dist = FVector::Dist2D(Gate->GetActorLocation(), Spawners[SpawnerNum]->GetActorLocation());
				if (dist <= LockRadius)
				{
					IBPI_GateControl::Execute_InternalLockGate(Gate);
				}
				//FString Msg = FString::Printf(TEXT("Distance between gate and spawner is %f!"), dist);
				//GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow, Msg);
			}
		}
	}
	
	if (AMiasmaManager* MiasmaManager = AMiasmaManager::Get())
	{
		MiasmaManager->RearrangeMiasma(SpawnersToUse, InvalidArea, CurrentWaveCount);
	}
	
	CurrentWaveCount++;
	
	// Set timer to show warning for next wave
	GetWorld()->GetTimerManager().SetTimer(
	WaveWarningTimerHandle,
	this,
	&AWaveSpawnerManager::ShowWarning,
	DefaultWaveSettings.SpawnDelay - DefaultWaveSettings.SpawnShowWarningTime,
	false
	);
}


void AWaveSpawnerManager::ShowWarning()
{
	if (!GetWorldTimerManager().IsTimerActive(WaveWarningTimerHandle))
		return;

	if (AProjectSwaggerHUD* GameHUD = Cast<AProjectSwaggerHUD>(GetWorld()->GetFirstPlayerController()->GetHUD()))
	{
		GameHUD->ShowWaveWarning(DefaultWaveSettings.SpawnShowWarningTime);
	}
}


void AWaveSpawnerManager::OnEnemyAttackReceived()
{
	if (FMath::FRandRange(0.f, 100.f) > HazardTriggerChance)
		return;

	TArray<AActor*> AllNodes;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ANPCNodeSlot::StaticClass(), AllNodes);

	Algo::RandomShuffle(AllNodes);

	int HazardsTriggered = 0;
	for (AActor* Actor : AllNodes)
	{
		if (HazardsTriggered >= MaxHazardsPerAttack)
			break;

		if (ANPCNodeSlot* Node = Cast<ANPCNodeSlot>(Actor))
		{
			if (Node->bHazardScheduled)
				continue;

			if (Node->bIsHazardActive)
				continue;
			
			if (Node->bIsDisabled)
				continue;

			if (!Node->bIsOccupied)
				continue;

			FString Msg = FString::Printf(TEXT("Starting hazard timer for node!"));
			GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow, Msg);
			
			Node->StartHazardTimer();
			HazardsTriggered++;
			
		}
	}
}

void AWaveSpawnerManager::OnDifficultyIncreasing()
{
	//only increase hazard difficulty once every spawner has increased difficulty
	NumSpawnersIncreasedDifficulty++;
	
	if (NumSpawnersIncreasedDifficulty < NumSpawnersInScene)
		return;

	NumSpawnersIncreasedDifficulty = 0; //reset for next step in progression
	HazardTriggerChance += HazardChanceIncreaseStep;
	MaxHazardsPerAttack += HazardCountIncreaseStep;
	
	FString Msg = FString::Printf(TEXT("Difficulty Increasing! Hazard chance is now %f , Max Hazards per Attack is now %i. "),  HazardTriggerChance, MaxHazardsPerAttack );
	GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow, Msg);

	HazardTriggerChance = FMath::Clamp(HazardTriggerChance, 0.f, 100.f);
	MaxHazardsPerAttack = FMath::Min(MaxHazardsPerAttack, static_cast<int>(ANPCNodeSlot::NumActiveNodes));
}
