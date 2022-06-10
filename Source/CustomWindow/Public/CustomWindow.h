// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Brushes/SlateColorBrush.h"
#include <functional>
#include "Rendering/RenderingCommon.h"
#include "UnrealClient.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SViewport.h"
#include "Widgets/SWidget.h"

#define ACTIVE_COLOR 80, 80, 80, 255
#define DISABLED_COLOR 40, 40, 40, 255

class FCanvasItem;
class FSceneViewport;
class FCanvasBoxItem;
class FCanvasTextItem;
class FCanvasTileItem;

struct FBoxData
{
	FName Name;
	FVector2D Position = FVector2D::ZeroVector;
	FVector2D Size = FVector2D::UnitVector;
	FLinearColor Color;
	float Thickness = 0.1f;
};

struct FTextData
{
	FName Name;
	FVector2D Position = FVector2D::ZeroVector;
	float FontSize = 1.f;
	FLinearColor Color;
	FString Message;
};

struct FTileData
{
	FName Name;
	FVector2D Position = FVector2D::ZeroVector;
	FVector2D Size = FVector2D::UnitVector;
	FLinearColor Color = FLinearColor::White;
	FString TexturePath;
};

class FCustomViewportClient : public FViewportClient
{
public:
	FLinearColor BackgroundColor;
	TMap<FName, FCanvasItem*> CanvasItems;
	TMap<FName, UTexture2D*> LoadedTextures;

	FCustomViewportClient();
	~FCustomViewportClient();

	virtual void Draw(FViewport* Viewport, FCanvas* Canvas) override;

	void AddBox(const FBoxData& Data);
	void AddText(const FTextData& Data);
	void AddTile(const FTileData& Data);
};

class SCustomViewport : public SViewport
{
	SLATE_BEGIN_ARGS(SCustomViewport)
	{
	}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs){}

	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

public:
	void SetSceneViewport(TSharedPtr<FSceneViewport> InSceneViewport);

	TSharedPtr<FSceneViewport> SceneViewport;
};

class FCustomWindowModule : public IModuleInterface
{
public:
	FSlateColorBrush ActiveColor;
	FSlateColorBrush DisabledColor;
	FCustomViewportClient* ViewportClient;

	TMap<FName, TTuple<TSharedPtr<SBorder>, TSharedPtr<SWidget>>> Settings;
	TSharedPtr<SOverlay> Overlay;
	FBoxData BoxData;
	FTextData TextData;
	FTileData TileData;

	FCustomWindowModule();

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	TSharedRef<SDockTab> CreateWindow(const FSpawnTabArgs& TabArgs);
	void CreateToggle(FName Name, TSharedPtr<SWidget> WidgetToAdd);
	TSharedPtr<SWidget> CreateBoxSettings();
	TSharedRef<SWidget> CreateTextSettings();
	TSharedRef<SWidget> CreateTileSettings();
	void Deselect();
	void SetOverlay(TSharedRef<SWidget> NewWidget);
	FReply AddBox();
	void Init();
	TSharedRef<SWidget> CreateTextEditBox(const FText& Name, const std::function<void(const FText& InText, ETextCommit::Type CommitType)> &TextCommitLambda);
	TSharedRef<SWidget> CreateColorBox(const FText& Name, const std::function<void(FLinearColor InColor)>& ColorSelectLambda);
	TSharedRef<SWidget> CreateAssetSelection(const FText& Name, const std::function<void(const FAssetData& InAsset)>& AssetSelectLambda);
	TSharedRef<SWidget> CreateButton(const FText& Name, FReply(FCustomWindowModule::*InFunc)());

	template<typename T, typename U>
	void CreateNumericField(TSharedRef<U> Box, const FText& Name, ETextJustify::Type NameJustification, float FillWidthName, float FillWidthValue,
		const std::function<void(T InValue, ETextCommit::Type CommitType)>& ValueCommittedLambda,
		const std::function<T()>& ValueUpdateLambda)
	{
		Box->AddSlot().FillWidth(FillWidthName).VAlign(EVerticalAlignment::VAlign_Center)
			.AttachWidget(SNew(STextBlock).Text(Name).Justification(NameJustification));
		Box->AddSlot().FillWidth(FillWidthValue)
			.AttachWidget(SNew(SNumericEntryBox<T>)
							.MinValue(0.f)
							.OnValueCommitted_Lambda(ValueCommittedLambda)
							.Value_Lambda(ValueUpdateLambda));
	}

	template<typename T>
	FReply Add()
	{
		return FReply::Unhandled();
	}

	template<>
	FReply Add<FCanvasBoxItem>()
	{
		ViewportClient->AddBox(BoxData);
		return FReply::Handled();
	}

	template<>
	FReply Add<FCanvasTextItem>()
	{
		ViewportClient->AddText(TextData);
		return FReply::Handled();
	}

	template<>
	FReply Add<FCanvasTileItem>()
	{
		ViewportClient->AddTile(TileData);
		return FReply::Handled();
	}
};