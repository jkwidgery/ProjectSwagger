#include "NPCs/NPCNodeSlot.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "Components/HealthComponent.h"
#include "Components/SphereComponent.h"
#include "Enemies/EnemyBase.h"
#include "Kismet/GameplayStatics.h"
#include "ProjectSwagger/ProjectSwaggerCharacter.h"
#include "Engine/DamageEvents.h"
#include "UI/ProgressBarWidget.h"
#include "Components/WidgetComponent.h"
#include "NPCs/NPCAIController.h"
#include "NPCs/NPCManager.h"
#include "Player/Inventory/InventoryComponent.h"
#include "UI/ProjectSwaggerHUD.h"


unsigned int ANPCNodeSlot::NumActiveNodes = 0;

void ANPCNodeSlot::StartHazardTimer()
{
	bHazardScheduled = true;
	float Delay = Hazard.GetNextNeedDelay();
	GetWorld()->GetTimerManager().SetTimer(
		Hazard.ResourceTimerHandle,
		this,
		&ANPCNodeSlot::TriggerHazard,
		Delay,
		false
	);
}

void ANPCNodeSlot::TriggerHazard()
{
	bHazardScheduled = false;
	if (Hazard.CurrentQuantityNeeded > 0 )
		return;
	
	Hazard.CurrentQuantityNeeded = Hazard.GetRandomQuantity();

	FString Msg = FString::Printf(TEXT("%s now needs %d of resource: %s"),
		*GetName(),
		Hazard.CurrentQuantityNeeded,
		*Hazard.ResourceTag.GetTagName().ToString()
	);

	bIsHazardActive = true;
	GEngine->AddOnScreenDebugMessage(-1, 60.f, FColor::Orange, Msg);

	if (ANPCAIController* AIController = Cast<ANPCAIController>(OccupantNPC->GetController()))
	{
		AIController->BehaviorTreeComponent->StopLogic(TEXT("NPC needs resources!"));
		AIController->RunBehaviorTree(Hazard.HazardBehaviorTree);
	}
}

ANPCNodeSlot::ANPCNodeSlot()
{
	PrimaryActorTick.bCanEverTick = true;

	NumActiveNodes = 0;

	NodeMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("NodeMesh"));
	SetRootComponent(NodeMeshComponent);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMesh.Succeeded())
	{
		NodeMeshComponent->SetStaticMesh(CylinderMesh.Object);
	}
	
	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));

	// Inventory Component
	InventoryComponent = CreateDefaultSubobject<UInventoryComponent>(TEXT("PlayerInventory"));
	InventoryComponent->SetCapacity(10000);

	
	InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
	InteractionSphere->SetupAttachment(RootComponent);

	FVector BoundsExtent = RootComponent->Bounds.BoxExtent;

	// Choose a horizontal offset — this is how far beyond the node's edge the sphere should extend
	float AdditionalRadius = 10.f;

	float Radius = FMath::Max(BoundsExtent.X, BoundsExtent.Y) + AdditionalRadius;

	// Apply to the sphere
	InteractionSphere->SetSphereRadius(Radius);
	InteractionSphere->SetCollisionProfileName(TEXT("Trigger"));
	InteractionSphere->SetGenerateOverlapEvents(true);

	DetectionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("DetectionSphere"));
	DetectionSphere->SetCollisionObjectType(ECC_WorldDynamic);
	DetectionSphere->SetupAttachment(RootComponent);
	DetectionSphere->SetCollisionObjectType(ECC_WorldDynamic);
	DetectionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	DetectionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	DetectionSphere->SetCollisionResponseToChannel(ECC_Enemy, ECR_Overlap);
	DetectionSphere->SetSphereRadius(600.f);
	DetectionSphere->SetGenerateOverlapEvents(true);
}

bool ANPCNodeSlot::HasRequiredResources(const AProjectSwaggerCharacter* Player, TArray<AResourceBase*>& ItemsToRemove) const
{
	if (!Player || !Player->GetInventory())
		return false;

	TArray<AResourceBase*> Inventory = Player->GetInventory()->GetInventoryContents();

	int32 TotalFound = 0;

	for (int32 i = 0; i < Inventory.Num(); ++i)
	{
		AResourceBase* Item = Inventory[i];
		
		if (!Item) continue;

		if (Item->GetResourceTag().MatchesTagExact(Hazard.ResourceTag))
		{
			++TotalFound;
			ItemsToRemove.Add(Item);
			if (Item->GetResourceTag().MatchesTagExact(Hazard.HealingResourceTag))
			{
				//DO NOT USE HEAL FUNCTION-- Node can heal from 0, so it's a weird edge case for the health component
				HealthComponent->CurrentHealth =  FMath::Clamp(
				HealthComponent->CurrentHealth + Hazard.HealthHealedPerResource,0.0f, HealthComponent->MaxHealth);
			}

		}

		if (TotalFound >= Hazard.CurrentQuantityNeeded)
			return true;
	}

	return false;
}

bool ANPCNodeSlot::TakeHealingResources(AProjectSwaggerCharacter* Player)
{
	if (!Player || !Player->GetInventory())
		return false;

	TArray<AResourceBase*> Inventory = Player->GetInventory()->GetInventoryContents();


	for (int32 i = 0; i < Inventory.Num(); ++i)
	{
		if (HealthComponent->CurrentHealth >= HealthComponent->MaxHealth)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green,
FString::Printf(TEXT("Health is full; cannot accept any more health resources.")));
			return true;
		}
		
		AResourceBase* Item = Inventory[i];
		
		if (!Item) continue;

		if (Item->GetResourceTag().MatchesTagExact(Hazard.HealingResourceTag))
		{
			bIsDisabled = false;
			
			Player->RemoveCarriedResource(Item);
			Item->IsCarried = true;
			Item->CarryingPlayer = this;
			Item->SetActorEnableCollision(false);
			Item->SetActorTickEnabled(true);
			Item->SetActorLocation(GetActorLocation());
			InventoryComponent->AddItem(Item);
			
			//DO NOT USE HEAL FUNCTION-- Node can heal from 0, so it's a weird edge case for the health component
			HealthComponent->CurrentHealth =  FMath::Clamp(
			HealthComponent->CurrentHealth + Hazard.HealthHealedPerResource,0.0f, HealthComponent->MaxHealth);
			
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green,
	FString::Printf(TEXT("Resource fulfilled %f health. Current health is now: %f."), Hazard.HealthHealedPerResource, HealthComponent->CurrentHealth));
		}
	}

	return false;
}
void ANPCNodeSlot::OnResourceDelivered(FGameplayTag& ResourceType, int32 Quantity)
{
	if (!ResourceType.MatchesTagExact(Hazard.ResourceTag)) return;

	Hazard.CurrentQuantityNeeded -= Quantity;
	if (Hazard.CurrentQuantityNeeded <= 0)
	{
		Hazard.CurrentQuantityNeeded = 0;

		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green,
			FString::Printf(TEXT("Resource fulfilled for %s. NPC resuming."), *GetName()));

		// Resume AI
		if (OccupantNPC)
		{
			if (AAIController* AI = Cast<AAIController>(OccupantNPC->GetController()))
			{
				AI->RunBehaviorTree(BehaviorTreeAsset);
			}
		}
		bIsHazardActive = false;
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow,
			FString::Printf(TEXT("%d more %s needed"), Hazard.CurrentQuantityNeeded,
				*Hazard.ResourceTag.GetTagName().ToString()));
	}
}

void ANPCNodeSlot::OnPlayerEnter_Implementation(AActor* OtherActor)
{
	// Optional default behavior — overrideable from BP
}

void ANPCNodeSlot::OnEnemyEnter_Implementation(AActor* OtherActor)
{
	// Optional default behavior — overrideable from BP
}


void ANPCNodeSlot::OnEnemyOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
								   UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
								   bool bFromSweep, const FHitResult& SweepResult)
{
	AEnemyBase* Enemy = Cast<AEnemyBase>(OtherActor);

	if (!Enemy)
		return;

	OnEnemyEnter(Enemy);
}

void ANPCNodeSlot::OnPlayerOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
								   UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
								   bool bFromSweep, const FHitResult& SweepResult)
{
	AProjectSwaggerCharacter* Player = Cast<AProjectSwaggerCharacter>(OtherActor);
	if (!Player)
		return;
	
	//if the node needs healing, always accepting health resource
	if (HealthComponent->CurrentHealth < HealthComponent->MaxHealth && !bIsHazardActive)
	{
		TakeHealingResources(Player);
	}
	
	if (Hazard.CurrentQuantityNeeded <= 0)
		return;

	OnPlayerEnter(Player);

	

	TArray<AResourceBase*> ItemsToRemove;
	if (HasRequiredResources(Player, ItemsToRemove))
	{
		TArray<AResourceBase*> Inventory = Player->GetInventory()->GetInventoryContents();
		for (auto Resource : ItemsToRemove)
		{
			//TODO: Make the resource lerp to the node, like the resource tank
			Player->RemoveCarriedResource(Resource);
			Resource->IsCarried = true;
			Resource->CarryingPlayer = this;
			Resource->SetActorEnableCollision(false);
			Resource->SetActorTickEnabled(true);
			Resource->SetActorLocation(GetActorLocation());
			InventoryComponent->AddItem(Resource);
		}
		
		OnResourceDelivered(Hazard.ResourceTag, Hazard.CurrentQuantityNeeded);
	}
	else
	{
		FString NeedMsg = FString::Printf(TEXT("%s needs %d of %s"),
			*GetName(), Hazard.CurrentQuantityNeeded,
			*Hazard.ResourceTag.GetTagName().ToString());

		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow, NeedMsg);
	}

}


void ANPCNodeSlot::BeginPlay()
{
	Super::BeginPlay();
	
	DetectionSphere->OnComponentBeginOverlap.AddDynamic(this, &ANPCNodeSlot::OnEnemyOverlap);
	InteractionSphere->OnComponentBeginOverlap.AddDynamic(this, &ANPCNodeSlot::OnPlayerOverlap);
	if (HealthComponent)
	{
		HealthComponent->OnDeath.AddDynamic(this, &ANPCNodeSlot::OnDeath);
		HealthComponent->OnHealthChanged.AddDynamic(this, &ANPCNodeSlot::OnHealthChanged);
		OnHealthChanged(HealthComponent->CurrentHealth, HealthComponent->CurrentMaxHealth);
	}
}

void ANPCNodeSlot::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	//DrawDebugSphere(GetWorld(), GetActorLocation(), InteractionSphere->GetScaledSphereRadius(), 16, FColor::Green);

	if (AProjectSwaggerHUD* GameHUD = Cast<AProjectSwaggerHUD>(GetWorld()->GetFirstPlayerController()->GetHUD()))
	{
		if (GameHUD->IsInUI())
		{
			// In UI, pause timer if isn't paused
			if (!GetWorldTimerManager().IsTimerPaused(Hazard.ResourceTimerHandle))
			{
				GetWorldTimerManager().PauseTimer(Hazard.ResourceTimerHandle);	
			}
		}
		else
		{
			// Outisde UI, unpause timer if paused
			if (GetWorldTimerManager().IsTimerPaused(Hazard.ResourceTimerHandle))
			{
				GetWorldTimerManager().UnPauseTimer(Hazard.ResourceTimerHandle);
			}
		}
	}
}

float ANPCNodeSlot::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
	AController* EventInstigator, AActor* DamageCauser)
{
	float DamageTaken = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	if (HealthComponent)
	{
		HealthComponent->TakeDamage(DamageTaken);
	}

	return DamageTaken;
}

void ANPCNodeSlot::OnHealthChanged(float CurrentHealth, float CurrentMaxHealth)
{		
	if (UWidgetComponent* Widget = Cast<UWidgetComponent>(GetComponentByClass(UWidgetComponent::StaticClass())))
	{
		if (UProgressBarWidget* HealthBar = Cast<UProgressBarWidget>(Widget->GetWidget()))
		{
			HealthBar->SetPercent(CurrentHealth / CurrentMaxHealth);
		}
	}
}

void ANPCNodeSlot::ApplySimpleDamage(float DamageAmount, AActor* DamageCauser)
{
	FDamageEvent DamageEvent; // Default generic damage event
	TakeDamage(DamageAmount, DamageEvent, nullptr, DamageCauser);
}


void ANPCNodeSlot::OnDeath()
{
	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, "Node Disabled!");
	bIsDisabled = true;
	bIsHazardActive = false;
	bHazardScheduled = false;
	Hazard.CurrentQuantityNeeded = 0;
	NumActiveNodes--;
	bIsOccupied = false;
	OnDisabled();
	
	if (!OccupantNPC)
		return;
	
	if (ANPCAIController* AIController = Cast<ANPCAIController>(OccupantNPC->GetController()))
	{
		AIController->BrainComponent->StopLogic(TEXT("Node has been destroyed."));
		AIController->RunBehaviorTree(NPCOldBehaviorTree);
		OccupantNPC->SetToNode(nullptr);
		
		AProjectSwaggerCharacter* Player = Cast<AProjectSwaggerCharacter>(
	UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
	
		OccupantNPC->FollowPlayer(Player);
		GetWorld()->GetTimerManager().ClearTimer(Hazard.ResourceTimerHandle);
		OccupantNPC = nullptr;
	}
}

void ANPCNodeSlot::Interact(AProjectSwaggerCharacter* Player)
{

	if (bIsDisabled)
	{
		return; //can't equip to a disabled node
	}
	if (bIsOccupied)
	{
		//we are just unassigning the current occupant
		if (ANPCAIController* AIController = Cast<ANPCAIController>(OccupantNPC->GetController()))
		{
			AIController->BehaviorTreeComponent->StopLogic(TEXT("Unassigning NPC from slot."));

			AIController->RunBehaviorTree(NPCOldBehaviorTree);
			// Set NPC to follow player
			OccupantNPC->SetToNode(nullptr);
			OccupantNPC->SetActorLocation(Player->GetActorLocation());
			OccupantNPC->FollowPlayer(Player);
			
			//TODO: reset node stats to what they were before NPC was equipped

			GetWorld()->GetTimerManager().ClearTimer(Hazard.ResourceTimerHandle);
		}
		bIsOccupied = false;
		NumActiveNodes--;
		return;
	}
 	if (ANPCManager* NPCMgr = ANPCManager::Get(GetWorld()))
	{
		//TODO: update this to allow player to choose which NPC to equip rather than random selection
 		
		OccupantNPC = NPCMgr->GetRandomFollowerNPC();
 		if (!OccupantNPC)
 		{
 			FString NPCWarning = FString::Printf(TEXT("No NPCs recruited!"));
 			GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green, NPCWarning);
 			return;
 		}
 		OccupantNPC->SetToNode(this);
 		OccupantNPC->FollowPlayer(nullptr);
 		bIsOccupied = true;
 		NumActiveNodes++;
 		//TODO: update stats based on NPC values, make it impossible to talk to/recruit NPC

		//start BHT
		if (ANPCAIController* AIController = Cast<ANPCAIController>(OccupantNPC->GetController()))
		{

			NPCOldBehaviorTree = AIController->BehaviorTreeComponent->GetCurrentTree();
			AIController->ClearFollowTarget();
			if (BehaviorTreeAsset)
			{
				AIController->RunBehaviorTree(BehaviorTreeAsset);

				if (UBlackboardComponent* BB = AIController->GetBlackboardComponent())
				{
					//write whatever data is needed to the AI can operate on the node
					BB->SetValueAsObject("AssignedNode", this); // Expose the node actor
				}
			}
		}
	}
}
