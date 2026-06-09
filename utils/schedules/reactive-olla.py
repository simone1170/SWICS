component_actions = {
    'simulate': True,
    'transcribe': False,
    'train': False,
    'run': False,
    'evaluate': False,
    'plot': False,
}

ids_config = ""

# Compare link adaptation WITHOUT OLLA (stock SWICS: pure CQI->MCS, no NACK downgrade)
# vs WITH OLLA (NACK lowers per-UE MCS offset, ACK raises it; targets ~10% BLER).
# At a moderate jammer power the reactive jammer should now drive the *commanded* MCS
# down only under OLLA -- the mechanism the professor's attack relies on.
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
    OLLA: 'false',
}
base_on = {**base_off, OLLA: 'true'}

attacks = {
    'Baseline': {},
    'Jammer': {'attackName': 'Jammer'},
    'ReactiveJammer': {'attackName': 'ReactiveJammer'},
}

scenarios = {
    'olla-off': ({}, attacks, base_off),
    'olla-on': ({}, attacks, base_on),
}
