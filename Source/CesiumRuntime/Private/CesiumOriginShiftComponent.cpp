// Copyright 2020-2023 CesiumGS, Inc. and Contributors

#include "CesiumOriginShiftComponent.h"
#include "CesiumGeoreference.h"
#include "CesiumGlobeAnchorComponent.h"
#include "CesiumSubLevelComponent.h"
#include "CesiumSubLevelSwitcherComponent.h"
#include "CesiumWgs84Ellipsoid.h"
#include "LevelInstance/LevelInstanceActor.h"

class ALevelInstance;

UCesiumOriginShiftComponent::UCesiumOriginShiftComponent() : Super() {
  this->PrimaryComponentTick.bCanEverTick = true;
  this->PrimaryComponentTick.TickGroup = ETickingGroup::TG_PrePhysics;
}

void UCesiumOriginShiftComponent::BeginPlay() {
  Super::BeginPlay();

  this->GlobeAnchor = nullptr;

  AActor* Owner = this->GetOwner();
  if (!Owner)
    return;

  this->GlobeAnchor =
      Owner->FindComponentByClass<UCesiumGlobeAnchorComponent>();
  if (!this->GlobeAnchor) {
    // A globe anchor is missing and required, so add one.
    this->GlobeAnchor =
        Cast<UCesiumGlobeAnchorComponent>(Owner->AddComponentByClass(
            UCesiumGlobeAnchorComponent::StaticClass(),
            false,
            FTransform::Identity,
            false));
    Owner->AddInstanceComponent(this->GlobeAnchor);
  }
}

void UCesiumOriginShiftComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction) {
  Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

  if (!this->GlobeAnchor)
    return;

  ACesiumGeoreference* Georeference = this->GlobeAnchor->ResolveGeoreference();

  if (!Georeference)
    return;

  UCesiumSubLevelSwitcherComponent* Switcher =
      Georeference->GetSubLevelSwitcher();
  if (!Switcher)
    return;

  const TArray<TWeakObjectPtr<ALevelInstance>>& Sublevels =
      Switcher->GetRegisteredSubLevelsWeak();

  if (Sublevels.Num() == 0) {
    // If we don't have any known sub-levels, bail quickly to save ourselves a
    // little work.
    return;
  }

  FVector ActorEcef = this->GlobeAnchor->GetEarthCenteredEarthFixedPosition();

  ALevelInstance* ClosestActiveLevel = nullptr;
  double ClosestLevelDistance = std::numeric_limits<double>::max();

  for (int32 i = 0; i < Sublevels.Num(); ++i) {
    ALevelInstance* Current = Sublevels[i].Get();
    if (!IsValid(Current))
      continue;

    UCesiumSubLevelComponent* SubLevelComponent =
        Current->FindComponentByClass<UCesiumSubLevelComponent>();
    if (!IsValid(SubLevelComponent))
      continue;

    if (!SubLevelComponent->GetEnabled())
      continue;

    FVector LevelEcef =
        UCesiumWgs84Ellipsoid::LongitudeLatitudeHeightToEarthCenteredEarthFixed(
            FVector(
                SubLevelComponent->GetOriginLongitude(),
                SubLevelComponent->GetOriginLatitude(),
                SubLevelComponent->GetOriginHeight()));

    double LevelDistance = FVector::Distance(LevelEcef, ActorEcef);
    if (LevelDistance < SubLevelComponent->GetLoadRadius() &&
        LevelDistance < ClosestLevelDistance) {
      ClosestActiveLevel = Current;
      ClosestLevelDistance = LevelDistance;
    }
  }

  Switcher->SetTargetSubLevel(ClosestActiveLevel);
}
