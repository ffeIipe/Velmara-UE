#include "UI/Widgets/Common/VelmaraCommonStyles.h"

UVelmaraTransparentButtonStyle::UVelmaraTransparentButtonStyle()
{
	bSingleMaterial = false;
	MinWidth = 0;
	MinHeight = 0;
	ButtonPadding = FMargin(0.0f);
	CustomPadding = FMargin(0.0f);

	FSlateBrush EmptyBrush;
	EmptyBrush.DrawAs = ESlateBrushDrawType::NoDrawType;

	NormalBase = EmptyBrush;
	NormalHovered = EmptyBrush;
	NormalPressed = EmptyBrush;
	SelectedBase = EmptyBrush;
	SelectedHovered = EmptyBrush;
	SelectedPressed = EmptyBrush;
	Disabled = EmptyBrush;
}
