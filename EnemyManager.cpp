#include "Enemies/EnemyManager.h"
#include "Enemies/EnemyBase.h"
#include "Kismet/GameplayStatics.h"
#include "Interactables/Events/EventMgr.h"

AEnemyManager* AEnemyManager::Instance = nullptr;


TArray<AEnemyBase*> AEnemyManager::ActiveEnemies;

AEnemyManager::AEnemyManager()
{
	PrimaryActorTick.bCanEverTick = false;

}

void AEnemyManager::BeginPlay()
{
	Super::BeginPlay();
	Instance = this;
}

void AEnemyManager::RegisterEnemy(AEnemyBase* Enemy)
{
	if (Enemy)
	{
		ActiveEnemies.Add(Enemy);

		if (const AEventMgr* EventMgr = AEventMgr::Get())
		{
			// Apply effects to enemy
			EventMgr->ApplyEffectsToEnemy(Enemy);
		}
	}
}

void AEnemyManager::UnregisterEnemy(AEnemyBase* Enemy)
{
	if (Enemy)
		ActiveEnemies.RemoveSwap(Enemy);
}

const TArray<AEnemyBase*>& AEnemyManager::GetAllEnemies()
{
	return ActiveEnemies;
}

void AEnemyManager::TempAdjustMaxHealthForAll(float Value, bool IsAdding)
{
	for (AEnemyBase* Enemy : ActiveEnemies)
	{
		Enemy->TempAdjustMaxHealth(Value, IsAdding);
	}
}

void AEnemyManager::AdjustMaxHealthForAll(float Value, bool IsAdding)
{
	for (AEnemyBase* Enemy : ActiveEnemies)
	{
		Enemy->AdjustMaxHealth(Value, IsAdding);
	}
}

void AEnemyManager::ResetTempMaxHealthForAll()
{
	for (AEnemyBase* Enemy : ActiveEnemies)
	{
		Enemy->ResetTempMaxHealth();
	}
}

AEnemyManager* AEnemyManager::Get(UWorld* World)
{
	return Instance;
}
