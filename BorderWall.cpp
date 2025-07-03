// Fill out your copyright notice in the Description page of Project Settings.


#include "Environment/BorderWall.h"

// Sets default values
ABorderWall::ABorderWall()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	WallMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WallMesh"));
	RootComponent = WallMesh;

	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));

	WallMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	WallMesh->SetCollisionResponseToAllChannels(ECR_Block);
}

// Called when the game starts or when spawned
void ABorderWall::BeginPlay()
{
	Super::BeginPlay();

	if (HealthComponent)
		HealthComponent->OnDeath.AddDynamic(this, &ABorderWall::OnDeath);
}

// Called every frame
void ABorderWall::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ABorderWall::OnDeath()
{
	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, "Wall Destroyed!");
	OnDisabled();
}

float ABorderWall::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
	AController* EventInstigator, AActor* DamageCauser)
{
	float DamageTaken = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	if (HealthComponent)
	{
		HealthComponent->TakeDamage(DamageTaken);
	}

	return DamageTaken;
}