// Fill out your copyright notice in the Description page of Project Settings.


#include "PredItemFactory.h"
#include "PredItem.h"

UPredItemFactory::UPredItemFactory()
{
    bEditAfterNew = true;
    bCreateNew = true;
    SupportedClass = UPredItem::StaticClass();
}

uint32 UPredItemFactory::GetMenuCategories() const
{
    return EAssetTypeCategories::Type::Misc;
}

UObject* UPredItemFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
    UPredItem* PredItem = NewObject<UPredItem>(InParent, InClass, InName, Flags);
    return PredItem;
}

FText UPredItemFactory::GetDisplayName() const
{
    return FText::FromString("PredItem");
}
