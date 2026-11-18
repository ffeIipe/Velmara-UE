#include "GAS/AbilityTasks/AbilityTask_WaitComboInput.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FComboInputStateTest, "TesisUE.Combat.ComboInputState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FComboInputStateTest::RunTest(const FString& Parameters)
{
	using EResult = FComboInputState::EResult;

	{
		FComboInputState S;
		S.Press();
		S.Release();
		TestTrue("Tap antes de la ventana espera", S.Evaluate() == EResult::Nothing);
		S.OpenWindow();
		TestTrue("Tap encadena al abrirse la ventana", S.Evaluate() == EResult::Chain);
		TestFalse("Es Tap", S.bIsHold);
	}
	{
		FComboInputState S;
		S.OpenWindow();
		S.Press();
		TestTrue("Apretar con la ventana abierta deja el golpe pendiente", S.Evaluate() == EResult::ChainPending);
		TestTrue("Pendiente se avisa una sola vez", S.Evaluate() == EResult::Nothing);
		S.HoldReached();
		S.Release();
		TestTrue("Hold encadena", S.Evaluate() == EResult::Chain);
		TestTrue("Soltar después del Hold no lo vuelve Tap", S.bIsHold);
	}
	{
		FComboInputState S;
		S.Press();
		S.HoldReached();
		TestTrue("Hold antes de la ventana espera", S.Evaluate() == EResult::Nothing);
		S.OpenWindow();
		TestTrue("Hold encadena al abrirse la ventana", S.Evaluate() == EResult::Chain);
	}
	{
		FComboInputState S;
		S.OpenWindow();
		S.Release();
		S.HoldReached();
		TestTrue("Sin apretar no encadena", S.Evaluate() == EResult::Nothing);
		TestTrue("Primer press cuenta", S.Press());
		TestFalse("Segundo press se ignora", S.Press());
	}

	return true;
}

#endif
