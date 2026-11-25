import unreal
for p in ['GA_SkeletonMage_Blink','GA_SkeletonMage_Reanimate']:
    c=unreal.get_default_object(unreal.load_asset('/Game/Blueprints/GAS/Abilities/Enemies/SkeletonMage/'+p).generated_class())
    for t in c.get_editor_property('ability_triggers'):
        print(p, t.get_editor_property('trigger_tag'), t.get_editor_property('trigger_source'))
    print(p, 'cooldown', c.get_editor_property('cooldown_gameplay_effect_class'))
