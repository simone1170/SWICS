component_actions = {
    'simulate': True,
    'transcribe': False,
    'train': False,
    'run': False,
    'evaluate': False,
    'plot': False,
}

ids_config = ""

# Focused verification at one effective power: does the commanded (gNB AMC/CQI) MCS
# get driven down by the reactive jammer, and how does its duty cycle compare to the
# constant broadband jammer at the same power?
base = {
    'use5g': 'true',
    'duration': 10,
    'attackSchedule': [(2, 8)],
    'attackName': '',
    'jammerInside': 'false',
    'jammerDirected': 'false',
    'jammerPower': 55,
    'bjp': 0,
    'dutyCycle': 1,
}

attacks = {
    'Baseline': {},
    'Jammer': {'attackName': 'Jammer'},
    'ReactiveJammer': {'attackName': 'ReactiveJammer'},
}

scenarios = {
    '5g': ({}, attacks, base),
}
