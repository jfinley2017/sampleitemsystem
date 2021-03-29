// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "AssetTypeCategories.h"
#include "PredItemFactory.generated.h"

/**
 * Factory for creating PredItems.
 */
UCLASS()
class PREDECESSOR_API UPredItemFactory : public UFactory
{
	GENERATED_BODY()
	
public:

    UPredItemFactory();

    // UFactory
    virtual FText GetDisplayName() const override;
	virtual uint32 GetMenuCategories() const override;
    virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
    // ~UFactory


};
