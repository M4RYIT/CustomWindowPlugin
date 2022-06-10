// Copyright Epic Games, Inc. All Rights Reserved.

#include "CustomWindow.h"

#include "AssetRegistry/AssetData.h"
#include "CanvasItem.h"
#include "CanvasTypes.h"
#include "Engine/Font.h"
#include "Fonts/SlateFontInfo.h"
#include "Framework/Docking/TabManager.h"
#include "PropertyCustomizationHelpers.h"
#include "Slate/SceneViewport.h"
#include "Widgets/Colors/SColorSpectrum.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SNumericEntryBox.h" 
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/STableRow.h"
#include "WorkspaceMenuStructureModule.h"
#include "WorkspaceMenuStructure.h"

#include <iostream>

#define LOCTEXT_NAMESPACE "FCustomWindowModule"

static const FName WindowDockTab("WindowDockTab");

FCustomWindowModule::FCustomWindowModule() 
: ActiveColor(FColor(ACTIVE_COLOR)), DisabledColor(FColor(DISABLED_COLOR)), BoxData(FBoxData())
{
}

void FCustomWindowModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(WindowDockTab, FOnSpawnTab::CreateRaw(this, &FCustomWindowModule::CreateWindow))
		.SetGroup(WorkspaceMenu::GetMenuStructure().GetDeveloperToolsMiscCategory());
}

void FCustomWindowModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(WindowDockTab);
	Settings.Empty();
	delete ViewportClient;
}

TSharedRef<SDockTab> FCustomWindowModule::CreateWindow(const FSpawnTabArgs& TabArgs)
{
	ViewportClient = new FCustomViewportClient();	
	TSharedPtr<SCustomViewport> Viewport = SNew(SCustomViewport);
	TSharedRef<FSceneViewport> Scene = MakeShared<FSceneViewport>(ViewportClient, Viewport);
	Viewport->SetViewportInterface(Scene);
	Viewport->SetSceneViewport(Scene);
	Viewport->SetCanTick(true);	

	Init();

	TSharedPtr<SVerticalBox> VBox = SNew(SVerticalBox);
	for (auto& Setting : Settings)
	{
		VBox->AddSlot().AttachWidget(Setting.Value.Key.ToSharedRef());
	}

	return SNew(SDockTab).TabRole(ETabRole::NomadTab)
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot().Padding(0.f, 5.f)
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot().FillWidth(0.2f).Padding(5.f, 0.f)
				[					
					VBox.ToSharedRef()
				]
				+SHorizontalBox::Slot().FillWidth(0.8f).Padding(5.f, 0.f)
				[
					Viewport.ToSharedRef()
				]
			]
			+SVerticalBox::Slot().AutoHeight().Padding(0.f, 5.f)
			[
				SNew(SBorder).BorderImage(&ActiveColor)
				[
					SNew(SBox).HeightOverride(FOptionalSize(150))
					[
						SAssignNew(Overlay, SOverlay)
					]
				]
			]
		];
}

void FCustomWindowModule::CreateToggle(FName Name, TSharedPtr<SWidget> WidgetToAdd)
{
	TSharedPtr<SBorder> Border = SNew(SBorder)
		.OnMouseButtonDown_Lambda([this, Name, WidgetToAdd](const FGeometry& Geom, const FPointerEvent&)->FReply
			{ Deselect(); Settings[Name].Key->SetBorderImage(&ActiveColor);
			  SetOverlay(WidgetToAdd.ToSharedRef()); return FReply::Handled(); })
		.VAlign(EVerticalAlignment::VAlign_Center)
		.BorderImage(&DisabledColor);

	Border->SetContent(SNew(STextBlock).Text(FText::FromName(Name)).Justification(ETextJustify::Center));
	Settings.Emplace(Name, MakeTuple(Border, WidgetToAdd));
}

void FCustomWindowModule::SetOverlay(TSharedRef<SWidget> NewWidget)
{
	if (Overlay->GetNumWidgets() > 0)
	{
		Overlay->RemoveSlot(0);		
	}	
	Overlay->AddSlot(0).AttachWidget(NewWidget);
}

void FCustomWindowModule::Deselect()
{
	for (auto& Setting : Settings)
	{		
		TSharedRef<SBorder> Border = Setting.Value.Key.ToSharedRef();
		Border->SetBorderImage(&DisabledColor);
	}
}

TSharedPtr<SWidget> FCustomWindowModule::CreateBoxSettings()
{
	auto OnNameCommit = [this](const FText& InText, ETextCommit::Type CommitType) -> void { BoxData.Name = FName(InText.ToString()); };
	
	auto OnThicknessCommit = [this](float InValue, ETextCommit::Type CommitType) -> void { BoxData.Thickness = InValue; };
	auto OnThicknessUpdate = [this]() -> float { return BoxData.Thickness; };
	TSharedRef<SHorizontalBox> ThicknessBox = SNew(SHorizontalBox);	
	CreateNumericField<float, SHorizontalBox>(ThicknessBox, FText::FromString("Thickness"), ETextJustify::Left, 0.2f, 0.8f, OnThicknessCommit, OnThicknessUpdate);

	auto OnPosXCommit = [this](uint32 InValue, ETextCommit::Type CommitType) -> void { BoxData.Position.X = InValue; };
	auto OnPosXUpdate = [this]() -> uint32 { return BoxData.Position.X; };
	auto OnPosYCommit = [this](uint32 InValue, ETextCommit::Type CommitType) -> void { BoxData.Position.Y = InValue; };
	auto OnPosYUpdate = [this]() -> uint32 { return BoxData.Position.Y; };
	TSharedRef<SHorizontalBox> PosBox = SNew(SHorizontalBox);
	CreateNumericField<uint32, SHorizontalBox>(PosBox, FText::FromString("X"), ETextJustify::Center, 0.1f, 0.4f, OnPosXCommit, OnPosXUpdate);
	CreateNumericField<uint32, SHorizontalBox>(PosBox, FText::FromString("Y"), ETextJustify::Center, 0.1f, 0.4f, OnPosYCommit, OnPosYUpdate);

	auto OnSizeWCommit = [this](uint32 InValue, ETextCommit::Type CommitType) -> void { BoxData.Size.X = InValue; };
	auto OnSizeWUpdate = [this]() -> uint32 { return BoxData.Size.X; };
	auto OnSizeHCommit = [this](uint32 InValue, ETextCommit::Type CommitType) -> void { BoxData.Size.Y = InValue; };
	auto OnSizeHUpdate = [this]() -> uint32 { return BoxData.Size.Y; };
	TSharedRef<SHorizontalBox> SizeBox = SNew(SHorizontalBox);
	CreateNumericField<uint32, SHorizontalBox>(SizeBox, FText::FromString("W"), ETextJustify::Center, 0.1f, 0.4f, OnSizeWCommit, OnSizeWUpdate);
	CreateNumericField<uint32, SHorizontalBox>(SizeBox, FText::FromString("H"), ETextJustify::Center, 0.1f, 0.4f, OnSizeHCommit, OnSizeHUpdate);

	auto OnColorSelected = [this](FLinearColor InColor) -> void { BoxData.Color = InColor.HSVToLinearRGB(); };

	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().Padding(0.f, 5.f)
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot().FillWidth(0.5f).Padding(5.f, 0.f) //Name hbox
			[
				CreateTextEditBox(FText::FromString("Name"), OnNameCommit)
			]
			+SHorizontalBox::Slot().FillWidth(0.5f).Padding(5.f, 0.f) //Pos hbox
			[
				PosBox
			]
		]
		+SVerticalBox::Slot().Padding(0.f, 5.f)
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot().FillWidth(0.5f).Padding(5.f, 0.f) //Thickness hbox
			[
				ThicknessBox
			]
			+SHorizontalBox::Slot().FillWidth(0.5f).Padding(5.f, 0.f) //Size hbox
			[
				SizeBox
			]
		]
		+SVerticalBox::Slot().Padding(0.f, 5.f)
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot().FillWidth(0.5f).Padding(5.f, 0.f) //Color hbox
			[
				CreateColorBox(FText::FromString("Color"), OnColorSelected)
			]
			+SHorizontalBox::Slot().FillWidth(0.5f).HAlign(EHorizontalAlignment::HAlign_Center)
			[
				CreateButton(FText::FromString("ENTER"), &FCustomWindowModule::Add<FCanvasBoxItem>)
			]
		];
}

TSharedRef<SWidget> FCustomWindowModule::CreateTextSettings()
{
	auto OnNameCommit = [this](const FText& InText, ETextCommit::Type CommitType) -> void { TextData.Name = FName(InText.ToString()); };
	auto OnMessageCommit = [this](const FText& InText, ETextCommit::Type CommitType) -> void { TextData.Message = InText.ToString(); };

	auto OnPosXCommit = [this](uint32 InValue, ETextCommit::Type CommitType) -> void { TextData.Position.X = InValue; };
	auto OnPosXUpdate = [this]() -> uint32 { return TextData.Position.X; };
	auto OnPosYCommit = [this](uint32 InValue, ETextCommit::Type CommitType) -> void { TextData.Position.Y = InValue; };
	auto OnPosYUpdate = [this]() -> uint32 { return TextData.Position.Y; };
	TSharedRef<SHorizontalBox> PosBox = SNew(SHorizontalBox);
	CreateNumericField<uint32, SHorizontalBox>(PosBox, FText::FromString("X"), ETextJustify::Center, 0.1f, 0.4f, OnPosXCommit, OnPosXUpdate);
	CreateNumericField<uint32, SHorizontalBox>(PosBox, FText::FromString("Y"), ETextJustify::Center, 0.1f, 0.4f, OnPosYCommit, OnPosYUpdate);

	auto OnFontSizeCommit = [this](float InValue, ETextCommit::Type CommitType) -> void { TextData.FontSize = InValue; };
	auto OnFontSizeUpdate = [this]() -> float { return TextData.FontSize; };
	TSharedRef<SHorizontalBox> FontSizeBox = SNew(SHorizontalBox);
	CreateNumericField<float, SHorizontalBox>(FontSizeBox, FText::FromString("Font Size"), ETextJustify::Left, 0.2f, 0.8f, OnFontSizeCommit, OnFontSizeUpdate);

	auto OnColorSelected = [this](FLinearColor InColor) -> void { TextData.Color = InColor.HSVToLinearRGB(); };

	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().Padding(0.f, 5.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(0.5f).Padding(5.f, 0.f) //Name hbox
			[
				CreateTextEditBox(FText::FromString("Name"), OnNameCommit)
			]
			+ SHorizontalBox::Slot().FillWidth(0.5f).Padding(5.f, 0.f) //Pos hbox
			[
				PosBox
			]
		]
		+ SVerticalBox::Slot().Padding(0.f, 5.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(0.5f).Padding(5.f, 0.f) //Thickness hbox
			[
				CreateTextEditBox(FText::FromString("Message"), OnMessageCommit)
			]
			+ SHorizontalBox::Slot().FillWidth(0.5f).Padding(5.f, 0.f) //Size hbox
			[
				FontSizeBox
			]
		]
		+ SVerticalBox::Slot().Padding(0.f, 5.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(0.5f).Padding(5.f, 0.f) //Color hbox
			[
				CreateColorBox(FText::FromString("Color"), OnColorSelected)
			]
			+ SHorizontalBox::Slot().FillWidth(0.5f).HAlign(EHorizontalAlignment::HAlign_Center)
			[
				CreateButton(FText::FromString("ENTER"), &FCustomWindowModule::Add<FCanvasTextItem>)
			]
		];
}

TSharedRef<SWidget> FCustomWindowModule::CreateTileSettings()
{
	auto OnNameCommit = [this](const FText& InText, ETextCommit::Type CommitType) -> void { TileData.Name = FName(InText.ToString()); };

	auto OnPosXCommit = [this](uint32 InValue, ETextCommit::Type CommitType) -> void { TileData.Position.X = InValue; };
	auto OnPosXUpdate = [this]() -> uint32 { return TileData.Position.X; };
	auto OnPosYCommit = [this](uint32 InValue, ETextCommit::Type CommitType) -> void { TileData.Position.Y = InValue; };
	auto OnPosYUpdate = [this]() -> uint32 { return TileData.Position.Y; };
	TSharedRef<SHorizontalBox> PosBox = SNew(SHorizontalBox);
	CreateNumericField<uint32, SHorizontalBox>(PosBox, FText::FromString("X"), ETextJustify::Center, 0.1f, 0.4f, OnPosXCommit, OnPosXUpdate);
	CreateNumericField<uint32, SHorizontalBox>(PosBox, FText::FromString("Y"), ETextJustify::Center, 0.1f, 0.4f, OnPosYCommit, OnPosYUpdate);

	auto OnSizeWCommit = [this](uint32 InValue, ETextCommit::Type CommitType) -> void { TileData.Size.X = InValue; };
	auto OnSizeWUpdate = [this]() -> uint32 { return TileData.Size.X; };
	auto OnSizeHCommit = [this](uint32 InValue, ETextCommit::Type CommitType) -> void { TileData.Size.Y = InValue; };
	auto OnSizeHUpdate = [this]() -> uint32 { return TileData.Size.Y; };
	TSharedRef<SHorizontalBox> SizeBox = SNew(SHorizontalBox);
	CreateNumericField<uint32, SHorizontalBox>(SizeBox, FText::FromString("W"), ETextJustify::Center, 0.1f, 0.4f, OnSizeWCommit, OnSizeWUpdate);
	CreateNumericField<uint32, SHorizontalBox>(SizeBox, FText::FromString("H"), ETextJustify::Center, 0.1f, 0.4f, OnSizeHCommit, OnSizeHUpdate);

	auto OnColorSelected = [this](FLinearColor InColor) -> void { TileData.Color = InColor.HSVToLinearRGB(); TileData.Color.A = 1.0f; };
	auto OnTextureSelected = [this](const FAssetData& InTexture) -> void { TileData.TexturePath = InTexture.ObjectPath.ToString(); };

	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().Padding(0.f, 5.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(0.5f).Padding(5.f, 0.f) //Name hbox
			[
				CreateTextEditBox(FText::FromString("Name"), OnNameCommit)
			]
			+ SHorizontalBox::Slot().FillWidth(0.5f).Padding(5.f, 0.f) //Pos hbox
			[
				PosBox
			]
		]
		+ SVerticalBox::Slot().Padding(0.f, 5.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(0.5f).Padding(5.f, 0.f) //Thickness hbox
			[
				CreateColorBox(FText::FromString("Color"), OnColorSelected)
			]
			+ SHorizontalBox::Slot().FillWidth(0.5f).Padding(5.f, 0.f) //Size hbox
			[
				SizeBox
			]
		]
		+ SVerticalBox::Slot().Padding(0.f, 5.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(0.5f).Padding(5.f, 0.f) //Color hbox
			[
				CreateAssetSelection(FText::FromString("Texture"), OnTextureSelected)
			]
			+ SHorizontalBox::Slot().FillWidth(0.5f).HAlign(EHorizontalAlignment::HAlign_Center)
			[
				CreateButton(FText::FromString("ENTER"), &FCustomWindowModule::Add<FCanvasTileItem>)
			]
		];
}

TSharedRef<SWidget> FCustomWindowModule::CreateButton(const FText& Name, FReply(FCustomWindowModule::*InFunc)())
{
	return SNew(SButton)
		.Text(Name)
		.OnClicked_Raw(this, InFunc)
		.VAlign(EVerticalAlignment::VAlign_Center);
}

TSharedRef<SWidget> FCustomWindowModule::CreateAssetSelection(const FText& Name, const std::function<void(const FAssetData& InAsset)>& AssetSelectLambda)
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(0.2f).VAlign(EVerticalAlignment::VAlign_Center)
		[
			SNew(STextBlock).Text(Name)
		]
		+ SHorizontalBox::Slot().FillWidth(0.8f)
		[
			SNew(SObjectPropertyEntryBox)
			.AllowedClass(UTexture2D::StaticClass())
			.DisplayThumbnail(true)
			.OnObjectChanged_Lambda(AssetSelectLambda)
		];
}

TSharedRef<SWidget> FCustomWindowModule::CreateTextEditBox(const FText& Name, const std::function<void(const FText& InText, ETextCommit::Type CommitType)> &TextCommitLambda)
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(0.2f).VAlign(EVerticalAlignment::VAlign_Center)
		[
			SNew(STextBlock).Text(Name)
		]
		+ SHorizontalBox::Slot().FillWidth(0.8f)
		[
			SNew(SEditableTextBox)
			.Text(FText::FromString("Enter..."))
			.OnTextCommitted_Lambda(TextCommitLambda)
		];
}

TSharedRef<SWidget> FCustomWindowModule::CreateColorBox(const FText& Name, const std::function<void(FLinearColor InColor)>& ColorSelectLambda)
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(0.2f).VAlign(EVerticalAlignment::VAlign_Center)
		[
			SNew(STextBlock).Text(Name)
		]
		+ SHorizontalBox::Slot().FillWidth(0.8f)
		[
			SNew(SColorSpectrum)
			.OnValueChanged_Lambda(ColorSelectLambda)
		];
}

FReply FCustomWindowModule::AddBox()
{
	ViewportClient->AddBox(BoxData);
	return FReply::Handled();
}

void FCustomWindowModule::Init()
{
	CreateToggle(FName("Box"), CreateBoxSettings());
	CreateToggle(FName("Text"), CreateTextSettings());
	CreateToggle(FName("Texture"), CreateTileSettings());
}

FCustomViewportClient::FCustomViewportClient() : BackgroundColor(FLinearColor::Black)
{
}

FCustomViewportClient::~FCustomViewportClient()
{
	CanvasItems.Empty();
	LoadedTextures.Empty();
}

void FCustomViewportClient::Draw(FViewport* Viewport, FCanvas* Canvas)
{
	Canvas->Clear(BackgroundColor);

	for (auto& Item : CanvasItems)
	{
		Canvas->DrawItem(*Item.Value);
	}
}

void FCustomViewportClient::AddBox(const FBoxData& Data)
{
	FCanvasBoxItem* Box = new FCanvasBoxItem(Data.Position, Data.Size);
	Box->SetColor(Data.Color);
	Box->LineThickness = Data.Thickness;

	if (CanvasItems.Contains(Data.Name))
	{
		delete CanvasItems[Data.Name];
	}

	CanvasItems.Emplace(Data.Name, Box);
}

void FCustomViewportClient::AddText(const FTextData& Data)
{
	FLinearColor Col = FLinearColor(Data.Color.R, Data.Color.G, Data.Color.B);
	FCanvasTextItem* Text = new FCanvasTextItem(Data.Position, FText::FromString(Data.Message), GEngine->GetSmallFont(), Col);
	Text->Scale = FVector2D(Data.FontSize);

	if (CanvasItems.Contains(Data.Name))
	{
		delete CanvasItems[Data.Name];
	}

	CanvasItems.Emplace(Data.Name, Text);
}

void FCustomViewportClient::AddTile(const FTileData& Data)
{
	UTexture2D* Texture;
	if (!LoadedTextures.Contains(FName(Data.TexturePath)))
	{
		Texture = LoadObject<UTexture2D>(nullptr, *Data.TexturePath);
		LoadedTextures.Emplace(FName(Data.TexturePath), Texture);
	}
	else
	{
		Texture = LoadedTextures[FName(Data.TexturePath)];
	}

	float TextureRatio = static_cast<float>(Texture->GetSizeX()) / Texture->GetSizeY();
	FCanvasTileItem* Tile = new FCanvasTileItem(Data.Position, Texture->GetResource(), FVector2D(Data.Size.X * TextureRatio, Data.Size.Y), Data.Color);
	Tile->BlendMode = ESimpleElementBlendMode::SE_BLEND_AlphaBlend;

	if (CanvasItems.Contains(Data.Name))
	{
		delete CanvasItems[Data.Name];
	}

	CanvasItems.Emplace(Data.Name, Tile);
}

void SCustomViewport::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	if (SceneViewport.IsValid())
	{
		SceneViewport->Invalidate();
	}
}

void SCustomViewport::SetSceneViewport(TSharedPtr<FSceneViewport> InSceneViewport)
{
	SceneViewport = InSceneViewport;
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FCustomWindowModule, CustomWindow)


