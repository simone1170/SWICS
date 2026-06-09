component_actions = {
    'simulate': True,
    'transcribe': False,
    'train': False,
    'run': False,
    'evaluate': False,
    'plot': True,
}

ids_config = ""

# Short 5G scenario to validate the MCS-downgrade attack.
# Attack window: 2s..8s of a 10s run.
base = {
    'use5g': 'true',
    'duration': 10,
    'attackSchedule': [(2, 8)],
    'attackName': '',
    'jammerInside': 'false',
    'jammerDirected': 'false',
    'jammerPower': 100,
    'bjp': 0,
    'dutyCycle': 1,
}

attacks = {
    'Baseline': {},                                  # reference, no attack
    'Jammer': {'attackName': 'Jammer'},              # constant/periodic jammer
    'ReactiveJammer': {'attackName': 'ReactiveJammer'},  # MCS-aware reactive jammer
}

# <name> : ({param: value_list}, attack_dict, default_config); {} => no param sweep
scenarios = {
    '5g': ({}, attacks, base),
}
