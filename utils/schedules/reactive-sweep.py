component_actions = {
    'simulate': True,
    'transcribe': False,
    'train': False,
    'run': False,
    'evaluate': False,
    'plot': False,   # plotting helper assumes >1 scenario row; we compute stats ourselves
}

ids_config = ""

# Jammer-power sweep to locate the MCS-DOWNGRADE regime (as opposed to full DoS).
# SWICS' own directed-jammer sweep transitions across ~20-40 dBm, so we anchor there.
# At each power we run the constant Jammer and the reactive MCS-aware Jammer so we can
# compare achieved MCS-downgrade / throughput-loss vs. jammer ON-time (energy).
base = {
    'use5g': 'true',
    'duration': 10,
    'attackSchedule': [(2, 8)],
    'attackName': '',
    'jammerInside': 'false',
    'jammerDirected': 'false',
    'jammerPower': 30,
    'bjp': 0,
    'dutyCycle': 1,
}

attacks = {
    'Baseline': {},   # run once at the lowest power as reference (identical every power)
    'Jammer': {'attackName': 'Jammer'},
    'ReactiveJammer': {'attackName': 'ReactiveJammer'},
}

# <name> : ({param: value_list}, attack_dict, default_config)
# Transition for this UE geometry is between 40 and 100 dBm; sweep that window.
scenarios = {
    '5g': ({'jammerPower': [45, 50, 55, 60, 70, 80, 90]}, attacks, base),
}
