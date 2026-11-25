import unreal

FOLDER = "/Game/Blueprints/Entities/Enemies/SolarBoss"
ENEMIES = "/Game/Blueprints/Entities/Enemies"
HIT_REACT_ABILITY = "/Game/Blueprints/GAS/Abilities/Player/GA_HitReact"
HIT_REACT_TAG = "Reaction.HitReact"
SWORD_MONTAGES = "/Game/Blueprints/Montages/Animations/Enemy/Sword"

eal = unreal.EditorAssetLibrary
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()


def blueprint_class(path):
    loaded = unreal.load_class(None, "%s.%s_C" % (path, path.rsplit("/", 1)[-1]))
    assert loaded, path
    return loaded


def load(path):
    loaded = unreal.load_asset(path)
    assert loaded, path
    return loaded


def tag(name):
    result = unreal.GameplayTag()
    result.import_text('(TagName="%s")' % name)
    assert name in result.export_text(), name
    return result


def tag_container(name):
    result = unreal.GameplayTagContainer()
    result.import_text('(GameplayTags=((TagName="%s")))' % name)
    assert name in result.export_text(), name
    return result


def get_or_create(name, asset_class, factory):
    path = "%s/%s" % (FOLDER, name)
    if eal.does_asset_exist(path):
        return load(path)
    created = asset_tools.create_asset(name, FOLDER, asset_class, factory)
    assert created, path
    return created


def get_or_create_blueprint(name, parent_class):
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", parent_class)
    return get_or_create(name, unreal.Blueprint, factory)


def save(asset):
    assert eal.save_loaded_asset(asset, False), asset.get_name()


def tag_hit_react_ability():
    ability = load(HIT_REACT_ABILITY)
    defaults = unreal.get_default_object(blueprint_class(HIT_REACT_ABILITY))
    if HIT_REACT_TAG not in defaults.get_editor_property("AbilityTags").export_text():
        assert "TagName" not in defaults.get_editor_property("AbilityTags").export_text()
        defaults.set_editor_property("AbilityTags", tag_container(HIT_REACT_TAG))
        unreal.BlueprintEditorLibrary.compile_blueprint(ability)
        save(ability)


def attack(name, minimum, maximum, cooldown, damage, montage=None, trigger=None):
    result = unreal.SolarBossAttackConfig()
    result.set_editor_property("AttackName", name)
    result.set_editor_property("MinDistance", minimum)
    result.set_editor_property("MaxDistance", maximum)
    result.set_editor_property("Cooldown", cooldown)
    result.set_editor_property("BaseDamage", damage)
    result.set_editor_property("DamageTypeTag", tag("Damage.Slash"))
    if montage:
        result.set_editor_property("Montage", load("%s/%s" % (SWORD_MONTAGES, montage)))
    if trigger:
        result.set_editor_property("AbilityTriggerTag", tag(trigger))
    return result


def build_config():
    factory = unreal.DataAssetFactory()
    factory.set_editor_property("data_asset_class", unreal.SolarBossConfigDataAsset)
    config = get_or_create("DA_SolarBossConfig", unreal.SolarBossConfigDataAsset, factory)
    config.set_editor_property("HealthBarClass", blueprint_class("/Game/Blueprints/HUD/WBP_PaladinBossHealthBar"))
    config.set_editor_property("DamageEffectClass", blueprint_class("/Game/Blueprints/GAS/Effects/GE_Damage"))
    config.set_editor_property("DamageSetByCallerTag", tag("Damage"))
    config.set_editor_property("HitReactAbilityTags", tag_container(HIT_REACT_TAG))
    config.set_editor_property("DefaultMinionClasses", [
        blueprint_class("%s/Skeleton/%s" % (ENEMIES, name))
        for name in ("CBP_Enemy_OrcMelee", "CBP_Enemy_MinerV1", "CBP_Enemy_Shaman")
    ])
    config.set_editor_property("MeleeAttacks", [
        attack("GAAttack", 0.0, 250.0, 1.5, 14.0, trigger="Ability.Attack"),
        attack("Slash3Combo", 0.0, 260.0, 2.5, 14.0, montage="AM_Enemy_Attack_3Combo_1"),
        attack("DashAttack", 250.0, 520.0, 5.0, 16.0, montage="AM_Enemy_Dash_Attack_ver_A"),
    ])
    save(config)
    return config


def blackboard_key(blackboard, name, key_class):
    entry = unreal.BlackboardEntry()
    entry.set_editor_property("EntryName", name)
    loaded_class = unreal.load_class(None, "/Script/AIModule.BlackboardKeyType_%s" % key_class)
    assert loaded_class, key_class
    key_type = unreal.new_object(loaded_class, outer=blackboard)
    if key_class == "Object":
        key_type.set_editor_property("BaseClass", unreal.Actor.static_class())
    if key_class == "Enum":
        enum_object = unreal.load_object(None, "/Script/TesisUE.ESolarBossMode")
        assert enum_object
        key_type.set_editor_property("EnumType", enum_object)
    entry.set_editor_property("KeyType", key_type)
    return entry


def build_blackboard():
    blackboard = get_or_create("BB_SolarBoss", unreal.BlackboardData, unreal.BlackboardDataFactory())
    keys = [
        ("TargetActor", "Object"),
        ("BossMode", "Enum"),
        ("MoveLocation", "Vector"),
        ("DistanceToTarget", "Float"),
        ("AliveMinionCount", "Int"),
        ("bEncounterStarted", "Bool"),
        ("bPendingCycleBreak", "Bool"),
        ("bNeedsDefensiveDodge", "Bool"),
        ("bNeedsEmergencySummon", "Bool"),
        ("bInEvadePanicRange", "Bool"),
    ]
    blackboard.set_editor_property("Keys", [blackboard_key(blackboard, name, kind) for name, kind in keys])
    save(blackboard)


def build_controller():
    controller = get_or_create_blueprint("AIC_SolarBoss", unreal.SolarBossAIController)
    unreal.BlueprintEditorLibrary.compile_blueprint(controller)
    tree = "%s/BT_SolarBoss" % FOLDER
    if eal.does_asset_exist(tree):
        unreal.get_default_object(blueprint_class("%s/AIC_SolarBoss" % FOLDER)).set_editor_property("BehaviorTreeAsset", load(tree))
        unreal.BlueprintEditorLibrary.compile_blueprint(controller)
    save(controller)


def build_boss(config):
    boss = get_or_create_blueprint("CBP_Enemy_SolarBoss", blueprint_class("%s/CBP_Enemy" % ENEMIES))
    subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    library = unreal.SubobjectDataBlueprintFunctionLibrary

    def brain_template():
        for handle in subsystem.k2_gather_subobject_data_for_blueprint(boss):
            found = library.get_object(library.get_data(handle))
            if isinstance(found, unreal.SolarBossBrainComponent):
                return found
        return None

    if not brain_template():
        root = subsystem.k2_gather_subobject_data_for_blueprint(boss)[0]
        handle, reason = subsystem.add_new_subobject(unreal.AddNewSubobjectParams(
            parent_handle=root, new_class=unreal.SolarBossBrainComponent, blueprint_context=boss))
        assert brain_template(), str(reason)
        subsystem.rename_subobject(handle, unreal.Text("SolarBossBrain"))

    brain_template().set_editor_property("Config", config)
    unreal.BlueprintEditorLibrary.compile_blueprint(boss)

    defaults = unreal.get_default_object(blueprint_class("%s/CBP_Enemy_SolarBoss" % FOLDER))
    defaults.set_editor_property("ai_controller_class", blueprint_class("%s/AIC_SolarBoss" % FOLDER))
    defaults.set_editor_property("auto_possess_ai", unreal.AutoPossessAI.PLACED_IN_WORLD_OR_SPAWNED)
    unreal.BlueprintEditorLibrary.compile_blueprint(boss)
    save(boss)

    mesh = defaults.get_editor_property("mesh")
    print("mesh", mesh.get_editor_property("skeletal_mesh_asset").get_name())
    print("anim", mesh.get_editor_property("anim_class").get_name())
    print("controller", defaults.get_editor_property("ai_controller_class").get_name())
    print("brain config", brain_template().get_editor_property("Config").get_name())


tag_hit_react_ability()
config = build_config()
build_blackboard()
build_controller()
build_boss(config)
print("assets", eal.list_assets(FOLDER))
