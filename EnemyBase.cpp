#include "Enemies/EnemyBase.h"
#include "Kismet/GameplayStatics.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Enemies/EnemyManager.h"
#include "GameEvents.h"
#include "UI/ProgressBarWidget.h"
#include "Engine/DamageEvents.h"
#include "UI/ProjectSwaggerHUD.h"
// Sets default values

AEnemyBase::AEnemyBase()
{
	PrimaryActorTick.bCanEverTick = true;

	VisualMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("VisualMesh"));
	RootComponent = VisualMesh;
	
	VisualMesh->SetCollisionObjectType(ECC_Enemy);

	// Ignore other enemies
	VisualMesh->SetCollisionResponseToChannel(ECC_Enemy, ECollisionResponse::ECR_Ignore);

	VisualMesh->SetCollisionResponseToChannel(ECC_Pawn, ECollisionResponse::ECR_Block);
	VisualMesh->SetCollisionResponseToChannel(ECC_WorldStatic, ECollisionResponse::ECR_Block);
	VisualMesh->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	float RandomScale = FMath::FRandRange(0.4f, 0.8f);
	VisualMesh->SetWorldScale3D(FVector(RandomScale));

	// Player Health Component
	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));
	HealthComponent->OnHealthChanged.AddDynamic(this, &AEnemyBase::OnHealthChanged);
	HealthComponent->OnDeath.AddDynamic(this, &AEnemyBase::OnDeath);

}

// Called when the game starts or when spawned
void AEnemyBase::BeginPlay()
{
	Super::BeginPlay();
	FindAndSetClosestWall();
}

// Called every frame
void AEnemyBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	if (AProjectSwaggerHUD* GameHUD = Cast<AProjectSwaggerHUD>(GetWorld()->GetFirstPlayerController()->GetHUD()))
	{
		if (GameHUD->IsInUI())
		{
			// In UI, don't tick
			return;	
		}
	}
	
	MoveTowardTarget(DeltaTime);
}

void AEnemyBase::OnHealthChanged(float CurrentHealth, float CurrentMaxHealth)
{
	if (UWidgetComponent* Widget = Cast<UWidgetComponent>(GetComponentByClass(UWidgetComponent::StaticClass())))
	{
		if (UProgressBarWidget* HealthBar = Cast<UProgressBarWidget>(Widget->GetWidget()))
		{
			HealthBar->SetPercent(CurrentHealth / CurrentMaxHealth);
		}
	}
}

void AEnemyBase::MoveTowardTarget(float DeltaTime)
{
    if (!TargetWall) return;

    FVector ClosestPoint;
	float Distance = TargetWall->GetWallMesh()->GetClosestPointOnCollision(GetActorLocation(), ClosestPoint);
	
    if (Distance > 0.f)
    {
    	TargetPoint = ClosestPoint;
    	//FString Message = FString::Printf(TEXT("Distance is %f"), Distance);
    	//UE_LOG(LogTemp, Display, TEXT("%s"), *Message);
        if (!FMath::IsWithin(Distance, 0.0f,  AttackRange))
        {
            FVector Direction = (ClosestPoint - GetActorLocation()).GetSafeNormal();
            AddActorWorldOffset(Direction * MoveSpeed * DeltaTime, true);
        	GetWorld()->GetTimerManager().ClearTimer(DamageTimerHandle);
        }
        else if (!bIsAttacking && DamageInterval > 0.f  && !GetWorld()->GetTimerManager().IsTimerActive(DamageTimerHandle))//fmath is nearly zero
        {
        	bIsAttacking = true;
        	GetWorld()->GetTimerManager().SetTimer(DamageTimerHandle, this, &AEnemyBase::Attack, DamageInterval, true);
        }
    }
}

void AEnemyBase::Attack() const
{
	if (!TargetWall) return;

	AProjectSwaggerHUD* GameHUD = Cast<AProjectSwaggerHUD>(GetWorld()->GetFirstPlayerController()->GetHUD());

	if (GameHUD->IsInUI())
	{
		GEngine->AddOnScreenDebugMessage(-1,1.5f, FColor::Cyan, TEXT("Enemy can't attack because UI!"));
		return;
	}

	float DistanceToWall = FVector::Dist(GetActorLocation(), TargetPoint);

	if (DistanceToWall <= AttackRange)
	{
		PlayAttackVFX();
		FDamageEvent DamageEvent;
		TargetWall->TakeDamage(10.0f, DamageEvent, nullptr, const_cast<AActor*>(Cast<AActor>(this)));

		UGameEvents::OnEnemyAttack.Broadcast();
	}
}

void AEnemyBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	AEnemyManager::UnregisterEnemy(this);
}

void AEnemyBase::DestroyedWall()
{
	GetWorld()->GetTimerManager().ClearTimer(DamageTimerHandle);
	FindAndSetClosestWall();
	bIsAttacking = false;
}

void AEnemyBase::AdjustMaxHealth(float Value, bool IsAdding)
{
	if (HealthComponent)
	{
		GEngine->AddOnScreenDebugMessage(-1,1.5f, FColor::Cyan, FString::Printf(TEXT("Object: %s"), *(GetName())));
		HealthComponent->AdjustMaxHealth(Value, IsAdding);
	}
}

void AEnemyBase::TempAdjustMaxHealth(float Value, bool IsAdding)
{
	if (HealthComponent)
	{
		GEngine->AddOnScreenDebugMessage(-1,1.5f, FColor::Cyan, FString::Printf(TEXT("Object: %s"), *(GetName())));
		HealthComponent->TempAdjustMaxHealth(Value, IsAdding);
	}
}

void AEnemyBase::ResetTempMaxHealth()
{
	if (HealthComponent)
	{
		GEngine->AddOnScreenDebugMessage(-1,1.5f, FColor::Cyan, FString::Printf(TEXT("Object: %s"), *(GetName())));
		HealthComponent->ResetTempMaxHealth();
	}
}

void AEnemyBase::FindAndSetClosestWall()
{
	TArray<AActor*> FoundWalls;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABorderWall::StaticClass(), FoundWalls);

	float ClosestDistanceSq = TNumericLimits<float>::Max();
	ABorderWall* ClosestWall = nullptr;
	FVector ClosestPoint = GetActorLocation();

	for (AActor* Actor : FoundWalls)
	{
		if (ABorderWall* Wall = Cast<ABorderWall>(Actor))
		{
			FVector PointOnWall = Wall->GetActorLocation(); // fallback
			if (UPrimitiveComponent* WallRoot = Cast<UPrimitiveComponent>(Wall->GetRootComponent()))
			{
				FVector OutClosestPoint;
				if (WallRoot->GetClosestPointOnCollision(GetActorLocation(), OutClosestPoint))
				{
					PointOnWall = OutClosestPoint;
				}
			}

			float DistanceSq = FVector::DistSquared(GetActorLocation(), PointOnWall);
			if (DistanceSq < ClosestDistanceSq)
			{
				ClosestDistanceSq = DistanceSq;
				ClosestWall = Wall;
				ClosestPoint = PointOnWall;
			}
		}
	}

	if (ClosestWall)
	{
		TargetWall = ClosestWall;
		TargetPoint = ClosestPoint;

		// Optionally: Move to TargetPoint
	}
}


float AEnemyBase::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
	AController* EventInstigator, AActor* DamageCauser)
{
	float DamageTaken = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	if (HealthComponent)
	{
		// Hit reaction
		if (VisualMesh && HitReactMontage)
		{
			UAnimInstance* AnimInstance = VisualMesh->GetAnimInstance();
			if (AnimInstance && !AnimInstance->Montage_IsPlaying(HitReactMontage))
			{
				AnimInstance->Montage_Play(HitReactMontage);
			}
		}
		
		HealthComponent->TakeDamage(DamageTaken);
	}

	return DamageTaken;
}

void AEnemyBase::ApplySimpleDamage(float DamageAmount, AActor* DamageCauser)
{
	FDamageEvent DamageEvent; // Default generic damage event
	TakeDamage(DamageAmount, DamageEvent, nullptr, DamageCauser);
}


void AEnemyBase::OnDeath()
{
	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, "Enemy Destroyed!");
}

