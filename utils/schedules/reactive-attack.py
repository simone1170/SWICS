component_actions = {
    'simulate': True,
    'transcribe': False,
    'train': False,
    'run': False,
    'evaluate': False,
    'plot': False,
}

ids_config = ""

# E4 -- WORKING ATTACK: idealized reactive effect ON (corrupt target high-MCS DL TBs ->
# NACK), compared OLLA off vs on. With OLLA on the target MCS collapses to the threshold;
# with OLLA off the same corruption cannot steer the MCS. Companion to reactive-olla.py
# (E2, ideal effect OFF, where the commanded MCS instead rises under jamming).
OLLA = 'ns3::MmWaveFlexTtiMacScheduler::UseOlla'

base_off = {
    'use5g': 'true',
    'duration': 10,
    'attackSchedule': [(2, 8)],
    'attackName': '',
    'jammerInside': 'false',
    'jammerDirected': 'false',
    'jammerPower': 55,
    'bjp': 0,
    'dutyCycle': 1,
    'ReactiveIdealEffect': 'true',   # GlobalValue: enable the idealized reactive effect
    OLLA: 'false',
}
base_on = {**base_off, OLLA: 'true'}

attacks = {
    'Baseline': {},
    'Jammer': {'attackName': 'Jammer'},
    'ReactiveJammer': {'attackName': 'ReactiveJammer'},
}

scenarios = {
    'ideal-olla-off': ({}, attacks, base_off),
    'ideal-olla-on': ({}, attacks, base_on),
}
